#include "inbox/InputProcessor.h"

#include "console_commands/ConsoleCommandProcessor.h"
#include "inbox/InboxFileReader.h"
#include "inbox/InboxState.h"
#include "inbox/StartupInboxClear.h"
#include "local_commands/LocalCommandProcessor.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include <Windows.h>

#include <variant>

namespace
{
	using namespace std::chrono_literals;

	void SwallowFailSoftBoundary() noexcept {}

	constexpr auto kPollInterval = 200ms;
	constexpr std::size_t kMaxNewBytesPerPoll = std::size_t{ 64 } * 1024;

	std::wstring TrimWide(std::wstring value)
	{
		const auto notSpace = [](wchar_t ch) {
			return !std::iswspace(ch);
		};

		const auto begin = std::find_if(value.begin(), value.end(), notSpace);
		const auto end = std::find_if(value.rbegin(), value.rend(), notSpace).base();
		if (begin >= end) {
			return L"";
		}

		return std::wstring(begin, end);
	}

	std::string ToUtf8(std::wstring_view value)
	{
		return SKSE::stl::utf16_to_utf8(value).value_or(std::string("<unicode conversion error>"));
	}

	std::string FormatWindowsError(DWORD errorCode)
	{
		wchar_t* messageBuffer = nullptr;
		const auto length = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&messageBuffer),
			0,
			nullptr);

		std::wstring message;
		if (length != 0 && messageBuffer != nullptr) {
			message.assign(messageBuffer, length);
		} else {
			message = L"Unknown Windows error";
		}

		if (messageBuffer != nullptr) {
			::LocalFree(messageBuffer);
		}

		return ToUtf8(TrimWide(std::move(message)));
	}

	std::string PathToString(const std::filesystem::path& path)
	{
		return ToUtf8(path.native());
	}

	std::string_view DescribeResetReason(const Inbox::ResetReason reason) noexcept
	{
		switch (reason) {
		case Inbox::ResetReason::kRemoved:
			return "was removed";
		case Inbox::ResetReason::kTruncated:
			return "was truncated";
		case Inbox::ResetReason::kReplaced:
			return "was atomically replaced";
		case Inbox::ResetReason::kOverwritten:
			return "was overwritten";
		case Inbox::ResetReason::kNone:
		default:
			return "";
		}
	}

	class InputProcessor
	{
	public:
		static InputProcessor& GetSingleton()
		{
			static InputProcessor processor;
			return processor;
		}

		void Initialize(const Settings::Values& settings, const std::filesystem::path& inboxPath)
		{
			auto expected = false;
			if (!_initialized.compare_exchange_strong(expected, true)) {
				return;
			}

			if (!settings.enableInbox) {
				logs::info("Inbox processing is disabled; inbox service will remain dormant");
				return;
			}

			_settings = settings;
			_inboxPath = inboxPath;

			LocalCommands::Initialize();
			ConsoleCommands::Initialize();

			logs::info("Inbox processing watching '{}'", PathToString(_inboxPath));
			StartWorker();
		}

	private:
		void StartWorker()
		{
			std::scoped_lock lock(_workerStartLock);
			if (!_workerStartState.CanStart(_serviceDisabled.load())) {
				return;
			}

			try {
				auto worker = std::jthread([this](const std::stop_token& stopToken) {
					try {
						if (!EstablishInboxReadiness()) {
							return;
						}
						PollLoop(stopToken);
					} catch (const std::exception& exception) {
						DisableService("worker thread failure", exception.what());
					} catch (...) {
						DisableService("worker thread failure", "unknown exception");
					}
				});

				_worker = std::move(worker);
				_workerStartState.MarkStarted();
				logs::info("Starting inbox processing worker thread");
			} catch (const std::exception& exception) {
				DisableService("worker startup failure", exception.what());
			} catch (...) {
				DisableService("worker startup failure", "unknown exception");
			}
		}

		[[nodiscard]] bool EstablishInboxReadiness()
		{
			const auto result = Inbox::ClearAtStartup(_inboxPath);
			const auto action = Inbox::DescribeStartupClearResult(result);
			if (!action.shouldProceed) {
				return FailStartupInboxClear(action, result.errorCode);
			}

			ClearFileErrorState();

			if (action.shouldResetState) {
				std::scoped_lock lock(_stateLock);
				_inboxState.ResetForNewSession();
			}

			Logging::LogLocalActivityLine(action.localActivityLine);
			return true;
		}

		void PollLoop(const std::stop_token& stopToken)
		{
			while (!stopToken.stop_requested() && !_serviceDisabled.load()) {
				PollInbox();
				ScheduleGameThreadTick();
				std::this_thread::sleep_for(kPollInterval);
			}
		}

		void PollInbox()
		{
			const Inbox::FileReader reader(_inboxPath);
			auto opened = reader.Open();
			if (std::holds_alternative<Inbox::FileMissing>(opened)) {
				HandleMissingInbox();
				return;
			}

			if (const auto* error = std::get_if<Inbox::FileError>(&opened)) {
				ReportFileError(error->operation, error->code);
				return;
			}

			auto& file = std::get<Inbox::OpenFile>(opened);
			ClearFileErrorState();
			const auto observeResult = ObserveInbox(file.Observation());
			switch (observeResult.readMode) {
			case Inbox::ReadMode::kNone:
				return;
			case Inbox::ReadMode::kAppendCandidate:
				ReadAppend(reader, file, observeResult.readOffset);
				return;
			case Inbox::ReadMode::kRewrite:
				ReadRewrite(reader, file);
				return;
			}
		}

		void HandleMissingInbox()
		{
			Inbox::UpdateResult updateResult{};
			{
				std::scoped_lock lock(_stateLock);
				updateResult = _inboxState.ObserveMissingFile();
			}
			LogInboxUpdate(updateResult);
			ClearFileErrorState();
		}

		Inbox::UpdateResult ObserveInbox(const Inbox::FileObservation& observation)
		{
			Inbox::UpdateResult observeResult{};
			{
				std::scoped_lock lock(_stateLock);
				observeResult = _inboxState.ObserveFile(observation);
			}
			LogInboxUpdate(observeResult);
			return observeResult;
		}

		void ReadAppend(const Inbox::FileReader& reader, const Inbox::OpenFile& file, const std::uint64_t readOffset)
		{
			const auto size = file.Observation().size;
			const auto bytesToRead = std::min<std::uint64_t>(size - readOffset, kMaxNewBytesPerPoll);
			auto read = file.ReadAt(0, readOffset + bytesToRead);
			if (const auto* error = std::get_if<Inbox::FileError>(&read)) {
				ReportFileError(error->operation, error->code);
				return;
			}

			const auto& observedContents = std::get<std::string>(read);
			if (static_cast<std::uint64_t>(observedContents.size()) != readOffset + bytesToRead) {
				return;
			}

			Inbox::UpdateResult appendResult{};
			{
				std::scoped_lock lock(_stateLock);
				appendResult = _inboxState.AppendObservedContents(readOffset, observedContents);
			}
			LogInboxUpdate(appendResult);

			if (appendResult.resetReason == Inbox::ResetReason::kOverwritten) {
				ReadRewrite(reader, file);
			}
		}

		void ReadRewrite(const Inbox::FileReader& reader, const Inbox::OpenFile& file)
		{
			const auto beforeRead = file.Observation();
			auto read = file.ReadAt(0, beforeRead.size);
			if (const auto* error = std::get_if<Inbox::FileError>(&read)) {
				ReportFileError(error->operation, error->code);
				return;
			}

			const auto& bytes = std::get<std::string>(read);
			if (bytes.size() != beforeRead.size) {
				return;
			}

			auto afterRead = reader.Open();
			if (const auto* error = std::get_if<Inbox::FileError>(&afterRead)) {
				ReportFileError(error->operation, error->code);
				return;
			}

			const auto* afterFile = std::get_if<Inbox::OpenFile>(&afterRead);
			if (afterFile == nullptr || !(beforeRead == afterFile->Observation())) {
				return;
			}

			Inbox::UpdateResult rewriteResult{};
			{
				std::scoped_lock lock(_stateLock);
				rewriteResult = _inboxState.AcceptRewriteContents(afterFile->Observation(), bytes);
			}
			LogInboxUpdate(rewriteResult);
		}

		void LogInboxUpdate(const Inbox::UpdateResult& result)
		{
			if (result.resetReason != Inbox::ResetReason::kNone) {
				if (result.droppedPendingLines > 0) {
					logs::info(
						"Inbox '{}' {}; dropped {} pending line(s) from the previous file contents",
						PathToString(_inboxPath),
						DescribeResetReason(result.resetReason),
						result.droppedPendingLines);
				} else {
					logs::info(
						"Inbox '{}' {}; reset file tracking for the rewritten inbox contents",
						PathToString(_inboxPath),
						DescribeResetReason(result.resetReason));
				}
			}

			if (result.extractedCount > 0) {
				logs::info(
					"Queued {} inbox line(s) from '{}'; pending={}",
					result.extractedCount,
					PathToString(_inboxPath),
					result.pendingCount);
			}

			if (result.queueAtCapacity) {
				if (!_loggedQueueFull) {
					logs::warn(
						"Pausing inbox ingestion for '{}' because the pending line queue reached {}; later inbox lines will not be ingested until the queue drains",
						PathToString(_inboxPath),
						_inboxState.MaxPendingLines());
					_loggedQueueFull = true;
				}
			} else {
				_loggedQueueFull = false;
			}

			if (result.oversizedFragmentBytes > 0) {
				if (!_loggedOversizedFragment) {
					logs::warn(
						"Discarding an oversized partial inbox line from '{}' after {} bytes without a newline",
						PathToString(_inboxPath),
						result.oversizedFragmentBytes);
					_loggedOversizedFragment = true;
				}
			} else if (result.pendingFragmentBytes == 0) {
				_loggedOversizedFragment = false;
			}
		}

		void ScheduleGameThreadTick()
		{
			if (_serviceDisabled.load()) {
				return;
			}

			if (_tickScheduled.exchange(true)) {
				return;
			}

			const auto* taskInterface = SKSE::GetTaskInterface();
			if (taskInterface == nullptr) {
				_tickScheduled = false;
				if (!_loggedMissingTaskInterface.exchange(true)) {
					logs::warn("Inbox processing is waiting for the SKSE task interface");
				}
				return;
			}

			_loggedMissingTaskInterface = false;
			taskInterface->AddTask([] {
				try {
					InputProcessor::GetSingleton().ProcessOnGameThread();
				} catch (const std::exception& exception) {
					InputProcessor::GetSingleton().DisableService("game-thread processing failure", exception.what());
				} catch (...) {
					InputProcessor::GetSingleton().DisableService("game-thread processing failure", "unknown exception");
				}
			});
		}

		void ProcessOnGameThread()
		{
			if (_serviceDisabled.load()) {
				_tickScheduled = false;
				return;
			}

			const auto lineBudget = std::max<std::uint32_t>(_settings.maxInboxLinesPerTick, 1);
			for (std::uint32_t index = 0; index < lineBudget; ++index) {
				std::optional<Inbox::PendingLine> line;
				{
					std::scoped_lock lock(_stateLock);
					auto front = _inboxState.FrontLine();
					if (!front.has_value()) {
						break;
					}

					line = std::move(*front);
					_inboxState.PopFrontLine(line->sequence);
				}

				if (LocalCommands::IsLocalCommandLine(line->text)) {
					static_cast<void>(LocalCommands::Process(line->text));
				} else {
					static_cast<void>(ConsoleCommands::Process(line->text));
				}
			}

			bool hasPendingLines = false;
			{
				std::scoped_lock lock(_stateLock);
				hasPendingLines = _inboxState.HasPendingLines();
			}

			_tickScheduled = false;
			if (hasPendingLines && !_serviceDisabled.load()) {
				ScheduleGameThreadTick();
			}
		}

		void DisableService(std::string_view context, std::string_view reason) noexcept
		{
			_serviceDisabled = true;
			_tickScheduled = false;
			if (_failureLogged.exchange(true)) {
				return;
			}

			try {
				logs::warn("Disabling inbox processing after {}: {}", context, reason);
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		}

		void ReportFileError(std::string_view operation, DWORD errorCode)
		{
			if (_lastFileErrorCode == errorCode && _lastFileErrorOperation == operation) {
				return;
			}

			_lastFileErrorCode = errorCode;
			_lastFileErrorOperation = std::string(operation);
			logs::warn(
				"Inbox processing {} failed for '{}': win32={} {}",
				operation,
				PathToString(_inboxPath),
				errorCode,
				FormatWindowsError(errorCode));
		}

		[[nodiscard]] bool FailStartupInboxClear(const Inbox::StartupClearAction& action, DWORD errorCode)
		{
			ReportFileError(action.failedOperation, errorCode);
			Logging::LogLocalActivityLine(action.localActivityLine);
			DisableService("clear failure", action.disableReason);
			return false;
		}

		void ClearFileErrorState()
		{
			_lastFileErrorCode = ERROR_SUCCESS;
			_lastFileErrorOperation.clear();
		}

		Settings::Values _settings{};
		std::filesystem::path _inboxPath;
		std::jthread _worker;
		std::mutex _workerStartLock;
		std::mutex _stateLock;
		Inbox::State _inboxState;
		Inbox::WorkerStartState _workerStartState;
		std::atomic_bool _initialized = false;
		std::atomic_bool _serviceDisabled = false;
		std::atomic_bool _failureLogged = false;
		std::atomic_bool _tickScheduled = false;
		std::atomic_bool _loggedMissingTaskInterface = false;
		bool _loggedQueueFull = false;
		bool _loggedOversizedFragment = false;
		DWORD _lastFileErrorCode = ERROR_SUCCESS;
		std::string _lastFileErrorOperation;
	};
}

namespace Inbox
{
	void Initialize(const Settings::Values& settings, const std::filesystem::path& inboxPath)
	{
		InputProcessor::GetSingleton().Initialize(settings, inboxPath);
	}
}
