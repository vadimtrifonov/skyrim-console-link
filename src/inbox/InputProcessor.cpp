#include "inbox/InputProcessor.h"

#include "console_commands/ConsoleCommandProcessor.h"
#include "inbox/InboxState.h"
#include "inbox/ZeroInbox.h"
#include "local_commands/LocalCommandProcessor.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include <Windows.h>

namespace
{
	using namespace std::chrono_literals;

	constexpr auto kPollInterval = 200ms;
	constexpr std::size_t kMaxReadBytesPerPoll = std::size_t{ 64 } * 1024;

	void SwallowFailSoftBoundary() noexcept {}

	bool TryGetFileIdentity(HANDLE handle, Inbox::InboxFileIdentity& identity, DWORD& errorCode) noexcept
	{
		BY_HANDLE_FILE_INFORMATION info{};
		if (!::GetFileInformationByHandle(handle, &info)) {
			errorCode = ::GetLastError();
			return false;
		}

		identity.volumeSerialNumber = info.dwVolumeSerialNumber;
		identity.fileIndexHigh = info.nFileIndexHigh;
		identity.fileIndexLow = info.nFileIndexLow;
		errorCode = ERROR_SUCCESS;
		return true;
	}

	class ScopedHandle
	{
	public:
		explicit ScopedHandle(HANDLE handle) noexcept :
			_handle(handle)
		{}

		~ScopedHandle() noexcept
		{
			if (_handle != INVALID_HANDLE_VALUE) {
				::CloseHandle(_handle);
			}
		}

		[[nodiscard]] HANDLE get() const noexcept
		{
			return _handle;
		}

	private:
		HANDLE _handle{ INVALID_HANDLE_VALUE };
	};

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

	std::string_view DescribeResetReason(const Inbox::InboxResetReason reason) noexcept
	{
		switch (reason) {
		case Inbox::InboxResetReason::kRemoved:
			return "was removed";
		case Inbox::InboxResetReason::kTruncated:
			return "was truncated";
		case Inbox::InboxResetReason::kReplaced:
			return "was atomically replaced";
		case Inbox::InboxResetReason::kNone:
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

		void Initialize(const Config::Settings& settings, const std::filesystem::path& inboxPath)
		{
			auto expected = false;
			if (!_initialized.compare_exchange_strong(expected, true)) {
				return;
			}

			if (!REL::Module::IsVR()) {
				logs::warn("Skipping inbox processing: the current implementation slice is VR-only");
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
						if (!EstablishZeroInbox()) {
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

		[[nodiscard]] bool EstablishZeroInbox()
		{
			const auto result = Inbox::EstablishZeroInboxFile(_inboxPath);
			const auto action = Inbox::DescribeZeroInboxResult(result);
			if (!action.shouldProceed) {
				return FailZeroInbox(action, result.errorCode);
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
			const auto handle = ScopedHandle(::CreateFileW(
				_inboxPath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr));

			if (handle.get() == INVALID_HANDLE_VALUE) {
				const auto error = ::GetLastError();
				if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
					Inbox::InboxUpdateResult updateResult{};
					{
						std::scoped_lock lock(_stateLock);
						updateResult = _inboxState.ObserveMissingFile();
					}
					LogInboxUpdate(updateResult);
					ClearFileErrorState();
					return;
				}

				ReportFileError("open", error);
				return;
			}

			LARGE_INTEGER fileSize{};
			if (!::GetFileSizeEx(handle.get(), &fileSize)) {
				ReportFileError("size", ::GetLastError());
				return;
			}

			Inbox::InboxFileIdentity identity{};
			DWORD identityError = ERROR_SUCCESS;
			if (!TryGetFileIdentity(handle.get(), identity, identityError)) {
				ReportFileError("identity", identityError);
				return;
			}

			ClearFileErrorState();

			const auto size = static_cast<std::uint64_t>(fileSize.QuadPart);
			Inbox::InboxUpdateResult observeResult{};
			{
				std::scoped_lock lock(_stateLock);
				observeResult = _inboxState.ObserveFile(identity, size);
			}
			LogInboxUpdate(observeResult);
			if (!observeResult.shouldRead) {
				return;
			}

			const auto bytesToRead = static_cast<DWORD>(std::min<std::uint64_t>(size - observeResult.readOffset, kMaxReadBytesPerPoll));
			LARGE_INTEGER offset{};
			offset.QuadPart = static_cast<LONGLONG>(observeResult.readOffset);
			if (!::SetFilePointerEx(handle.get(), offset, nullptr, FILE_BEGIN)) {
				ReportFileError("seek", ::GetLastError());
				return;
			}

			std::string chunk(bytesToRead, '\0');
			DWORD bytesRead = 0;
			if (!::ReadFile(handle.get(), chunk.data(), bytesToRead, &bytesRead, nullptr)) {
				ReportFileError("read", ::GetLastError());
				return;
			}

			if (bytesRead == 0) {
				return;
			}

			chunk.resize(bytesRead);

			Inbox::InboxUpdateResult appendResult{};
			{
				std::scoped_lock lock(_stateLock);
				appendResult = _inboxState.AppendChunk(chunk);
			}
			LogInboxUpdate(appendResult);
		}

		void LogInboxUpdate(const Inbox::InboxUpdateResult& result)
		{
			if (result.resetReason != Inbox::InboxResetReason::kNone) {
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
						"Pausing inbox ingestion for '{}' because the pending line queue reached {}; buffered lines will wait until the queue drains",
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

			ConsoleCommands::PollReadiness();

			const auto lineBudget = std::max<std::uint32_t>(_settings.maxInboxLinesPerTick, 1);
			for (std::uint32_t index = 0; index < lineBudget; ++index) {
				std::optional<Inbox::PendingInputLine> line;
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

		[[nodiscard]] bool FailZeroInbox(const Inbox::ZeroInboxAction& action, DWORD errorCode)
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

		Config::Settings _settings{};
		std::filesystem::path _inboxPath;
		std::jthread _worker;
		std::mutex _workerStartLock;
		std::mutex _stateLock;
		Inbox::InboxState _inboxState;
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
	void Initialize(const Config::Settings& settings, const std::filesystem::path& inboxPath)
	{
		InputProcessor::GetSingleton().Initialize(settings, inboxPath);
	}
}
