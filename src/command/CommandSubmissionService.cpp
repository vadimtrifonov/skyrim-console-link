#include "command/CommandSubmissionService.h"

#include "capture/ConsoleInputCapture.h"
#include "command/LocalCommandDispatcher.h"
#include "command/CommandSubmissionState.h"
#include "pch.h"

#include <Windows.h>

namespace
{
	using namespace std::chrono_literals;

	constexpr auto kPollInterval = 200ms;
	constexpr auto kDeferredRetryInterval = 1s;
	constexpr auto kMaxReadBytesPerPoll = 64 * 1024;

	std::uint64_t GetSteadyTickMillis() noexcept
	{
		return static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch())
				.count());
	}

	bool TryGetFileIdentity(HANDLE handle, Command::InboxFileIdentity& identity, DWORD& errorCode) noexcept
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
		wchar_t*   messageBuffer = nullptr;
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

	std::string_view DescribeResetReason(const Command::InboxResetReason reason) noexcept
	{
		switch (reason) {
		case Command::InboxResetReason::kRemoved:
			return "was removed";
		case Command::InboxResetReason::kTruncated:
			return "was truncated";
		case Command::InboxResetReason::kReplaced:
			return "was atomically replaced";
		case Command::InboxResetReason::kNone:
		default:
			return "";
		}
	}

	class CommandSubmissionService
	{
	public:
		static CommandSubmissionService& GetSingleton()
		{
			static CommandSubmissionService service;
			return service;
		}

		void Initialize(const Config::Settings& settings, const std::filesystem::path& inboxPath)
		{
			auto expected = false;
			if (!_initialized.compare_exchange_strong(expected, true)) {
				return;
			}

			if (!REL::Module::IsVR()) {
				logs::warn("Skipping console command submission: the first implementation slice is VR-only");
				return;
			}

			if (!settings.enableConsoleCommandSubmission) {
				logs::info("Console command submission is disabled; inbox service will remain dormant");
				return;
			}

			if (!Capture::Input::CanSubmitCommands()) {
				logs::warn("Skipping console command submission: the validated ExecuteCommand path is unavailable");
				return;
			}

			_settings = settings;
			_inboxPath = inboxPath;

			const auto* messaging = SKSE::GetMessagingInterface();
			if (messaging == nullptr) {
				logs::warn("Skipping console command submission: SKSE messaging interface is unavailable");
				return;
			}

			if (!messaging->RegisterListener(MessageHandler)) {
				logs::warn("Skipping console command submission: failed to register the SKSE messaging listener");
				return;
			}

			logs::info("Console command submission watching '{}'", PathToString(_inboxPath));
			StartWorker();
		}

	private:
		static void MessageHandler(SKSE::MessagingInterface::Message* message)
		{
			if (message != nullptr) {
				GetSingleton().HandleMessage(*message);
			}
		}

		void HandleMessage(const SKSE::MessagingInterface::Message& message)
		{
			if (message.type == SKSE::MessagingInterface::kDataLoaded) {
				_dataLoaded = true;
				StartWorker();
			}
		}

		void StartWorker()
		{
			std::scoped_lock lock(_workerStartLock);
			if (!_workerStartState.CanStart(_serviceDisabled.load())) {
				return;
			}

			try {
				auto worker = std::jthread([this](std::stop_token stopToken) {
					try {
						PollLoop(stopToken);
					} catch (const std::exception& exception) {
						DisableService("worker thread failure", exception.what());
					} catch (...) {
						DisableService("worker thread failure", "unknown exception");
					}
				});

				_worker = std::move(worker);
				_workerStartState.MarkStarted();
				logs::info("Starting console command submission worker thread");
			} catch (const std::exception& exception) {
				DisableService("worker startup failure", exception.what());
			} catch (...) {
				DisableService("worker startup failure", "unknown exception");
			}
		}

		void PollLoop(std::stop_token stopToken)
		{
			while (!stopToken.stop_requested() && !_serviceDisabled.load()) {
				PollInbox();
				ScheduleFlushIfNeeded();
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
					Command::InboxUpdateResult updateResult{};
					{
						std::scoped_lock lock(_submissionStateLock);
						updateResult = _inboxState.ObserveMissingFile();
						if (updateResult.resetReason != Command::InboxResetReason::kNone) {
							_nextFlushAttemptAt = 0;
						}
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

			Command::InboxFileIdentity identity{};
			DWORD                      identityError = ERROR_SUCCESS;
			if (!TryGetFileIdentity(handle.get(), identity, identityError)) {
				ReportFileError("identity", identityError);
				return;
			}

			ClearFileErrorState();

			const auto size = static_cast<std::uint64_t>(fileSize.QuadPart);
			Command::InboxUpdateResult observeResult{};
			{
				std::scoped_lock lock(_submissionStateLock);
				observeResult = _inboxState.ObserveFile(identity, size);
				if (observeResult.resetReason != Command::InboxResetReason::kNone) {
					_nextFlushAttemptAt = 0;
				}
			}
			LogInboxUpdate(observeResult);
			if (!observeResult.shouldRead) {
				return;
			}

			const auto    bytesToRead = static_cast<DWORD>(std::min<std::uint64_t>(size - observeResult.readOffset, kMaxReadBytesPerPoll));
			LARGE_INTEGER offset{};
			offset.QuadPart = static_cast<LONGLONG>(observeResult.readOffset);
			if (!::SetFilePointerEx(handle.get(), offset, nullptr, FILE_BEGIN)) {
				ReportFileError("seek", ::GetLastError());
				return;
			}

			std::string chunk(bytesToRead, '\0');
			DWORD       bytesRead = 0;
			if (!::ReadFile(handle.get(), chunk.data(), bytesToRead, &bytesRead, nullptr)) {
				ReportFileError("read", ::GetLastError());
				return;
			}

			if (bytesRead == 0) {
				return;
			}

			chunk.resize(bytesRead);

			Command::InboxUpdateResult appendResult{};
			{
				std::scoped_lock lock(_submissionStateLock);
				appendResult = _inboxState.AppendChunk(chunk);
			}
			LogInboxUpdate(appendResult);
		}

		void LogInboxUpdate(const Command::InboxUpdateResult& result)
		{
			if (result.resetReason != Command::InboxResetReason::kNone) {
				if (result.droppedPendingCommands > 0) {
					logs::info(
						"Command inbox '{}' {}; cleared {} queued command(s) and reset the read cursor",
						PathToString(_inboxPath),
						DescribeResetReason(result.resetReason),
						result.droppedPendingCommands);
				} else {
					logs::info(
						"Command inbox '{}' {}; reset the read cursor",
						PathToString(_inboxPath),
						DescribeResetReason(result.resetReason));
				}
			}

			if (result.extractedCount > 0) {
				logs::info(
					"Queued {} command inbox line(s) from '{}'; pending={}",
					result.extractedCount,
					PathToString(_inboxPath),
					result.pendingCount);
			}

			if (result.queueAtCapacity) {
				if (!_loggedPendingQueueFull) {
					logs::warn(
						"Pausing command inbox ingestion for '{}' because the pending command queue reached {}; buffered lines will wait until the queue drains",
						PathToString(_inboxPath),
						_inboxState.MaxPendingCommands());
					_loggedPendingQueueFull = true;
				}
			} else {
				_loggedPendingQueueFull = false;
			}

			if (result.oversizedFragmentBytes > 0) {
				if (!_loggedOversizedFragment) {
					logs::warn(
						"Discarding an oversized partial command inbox line from '{}' after {} bytes without a newline",
						PathToString(_inboxPath),
						result.oversizedFragmentBytes);
					_loggedOversizedFragment = true;
				}
			} else if (result.pendingFragmentBytes == 0) {
				_loggedOversizedFragment = false;
			}
		}

		void ScheduleFlushIfNeeded()
		{
			if (_serviceDisabled.load()) {
				return;
			}

			if (!HasPendingCommands()) {
				return;
			}

			const auto nextFlushAttemptAt = _nextFlushAttemptAt.load();
			if (nextFlushAttemptAt != 0 && GetSteadyTickMillis() < nextFlushAttemptAt) {
				return;
			}

			if (_flushScheduled.exchange(true)) {
				return;
			}

			const auto* taskInterface = SKSE::GetTaskInterface();
			if (taskInterface == nullptr) {
				_flushScheduled = false;
				if (!_loggedMissingTaskInterface.exchange(true)) {
					logs::warn("Console command submission is waiting for the SKSE task interface");
				}
				return;
			}

			_loggedMissingTaskInterface = false;
			taskInterface->AddTask([] {
				try {
					CommandSubmissionService::GetSingleton().FlushOnGameThread();
				} catch (const std::exception& exception) {
					CommandSubmissionService::GetSingleton().DisableService("game-thread flush failure", exception.what());
				} catch (...) {
					CommandSubmissionService::GetSingleton().DisableService("game-thread flush failure", "unknown exception");
				}
			});
		}

		bool HasPendingCommands()
		{
			std::scoped_lock lock(_submissionStateLock);
			return _inboxState.HasPendingCommands();
		}

		std::size_t GetPendingCommandCount()
		{
			std::scoped_lock lock(_submissionStateLock);
			return _inboxState.PendingCommandCount();
		}

		void FlushOnGameThread()
		{
			if (_serviceDisabled.load()) {
				_flushScheduled = false;
				return;
			}

			const auto commandBudget = std::max<std::uint32_t>(_settings.maxCommandsPerTick, 1);
			const auto queuedCount = GetPendingCommandCount();
			if (queuedCount > 0) {
				logs::info("Attempting to submit {} queued console command(s)", std::min<std::size_t>(queuedCount, commandBudget));
			}

			std::size_t submittedCount = 0;
			bool        deferred = false;
			bool        terminalFailure = false;
			std::string terminalFailureContext;
			std::string terminalFailureReason;
			for (std::uint32_t index = 0; index < commandBudget; ++index) {
				Command::PendingCommand pendingCommand{};
				bool                    hasPendingCommand = false;
				{
					std::scoped_lock submissionLock(_submissionStateLock);
					if (_serviceDisabled.load()) {
						_flushScheduled = false;
						return;
					}

					const auto front = _inboxState.FrontCommand();
					if (!front.has_value()) {
						break;
					}

					pendingCommand = std::move(*front);
					hasPendingCommand = true;
				}

				if (!hasPendingCommand) {
					break;
				}

				const auto localCommandResult = Command::TryDispatchLocalCommand(pendingCommand.text, _dataLoaded.load());
				switch (localCommandResult) {
				case Command::LocalCommandDispatchResult::kHandled:
					{
						std::scoped_lock submissionLock(_submissionStateLock);
						_inboxState.PopFrontCommand(pendingCommand.sequence);
					}
					++submittedCount;
					continue;
				case Command::LocalCommandDispatchResult::kDeferred:
					deferred = true;
					break;
				case Command::LocalCommandDispatchResult::kTerminalFailure:
					terminalFailure = true;
					terminalFailureContext = "terminal local command failure";
					terminalFailureReason = "a local command failed; inbox submission is parked for this session";
					break;
				case Command::LocalCommandDispatchResult::kNotHandled:
				default:
					break;
				}

				if (deferred || terminalFailure) {
					break;
				}

				const auto result = Capture::Input::SubmitCommand(pendingCommand.text);
				if (result == Capture::Input::SubmitCommandResult::kSubmitted) {
					std::scoped_lock submissionLock(_submissionStateLock);
					_inboxState.PopFrontCommand(pendingCommand.sequence);
				}

				switch (result) {
				case Capture::Input::SubmitCommandResult::kSubmitted:
					++submittedCount;
					break;
				case Capture::Input::SubmitCommandResult::kDeferred:
					deferred = true;
					break;
				case Capture::Input::SubmitCommandResult::kTerminalFailure:
					terminalFailure = true;
					terminalFailureContext = "terminal command submission failure";
					terminalFailureReason = "the validated ExecuteCommand path is no longer usable";
					break;
				}

				if (deferred || terminalFailure) {
					break;
				}
			}

			if (terminalFailure) {
				DisableService(terminalFailureContext, terminalFailureReason);
				return;
			}

			if (deferred) {
				_nextFlushAttemptAt = GetSteadyTickMillis() +
					static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(kDeferredRetryInterval).count());
			} else {
				_nextFlushAttemptAt = 0;
			}

			_flushScheduled = false;
			if (submittedCount > 0 && !deferred) {
				ScheduleFlushIfNeeded();
			}
		}

		void DisableService(std::string_view context, std::string_view reason) noexcept
		{
			_serviceDisabled = true;
			_flushScheduled = false;

			if (_failureLogged.exchange(true)) {
				return;
			}

			try {
				logs::warn("Disabling console command submission service after {}: {}", context, reason);
			} catch (...) {
				// Intentionally swallow logger failures so async boundaries remain fail-soft.
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
				"Console command submission {} failed for '{}': win32={} {}",
				operation,
				PathToString(_inboxPath),
				errorCode,
				FormatWindowsError(errorCode));
		}

		void ClearFileErrorState()
		{
			_lastFileErrorCode = ERROR_SUCCESS;
			_lastFileErrorOperation.clear();
		}

		Config::Settings      _settings{};
		std::filesystem::path _inboxPath;
		std::jthread          _worker;
		std::mutex            _workerStartLock;
		std::mutex            _submissionStateLock;
		Command::InboxState   _inboxState;
		Command::WorkerStartState _workerStartState;
		std::atomic_bool         _initialized = false;
		std::atomic_bool         _serviceDisabled = false;
		std::atomic_bool         _failureLogged = false;
		std::atomic_bool         _flushScheduled = false;
		std::atomic_bool         _dataLoaded = false;
		std::atomic_uint64_t     _nextFlushAttemptAt = 0;
		std::atomic_bool         _loggedMissingTaskInterface = false;
		bool                     _loggedPendingQueueFull = false;
		bool                     _loggedOversizedFragment = false;
		DWORD                    _lastFileErrorCode = ERROR_SUCCESS;
		std::string              _lastFileErrorOperation;
	};
}

namespace Command
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& inboxPath)
	{
		CommandSubmissionService::GetSingleton().Initialize(settings, inboxPath);
	}
}
