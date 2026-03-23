#include "display/DisplayService.h"

// clang-format off
#include "pch.h"
#include "RE/C/ConsoleLog.h"
#include "console/ConsoleActivityPolicy.h"
#include "display/DisplayState.h"
#include "logging/ConsoleActivityLog.h"
// clang-format on

#include <Windows.h>
#include <cstdarg>

namespace
{
	using namespace std::chrono_literals;

	constexpr auto        kPollInterval = 200ms;
	constexpr std::size_t kMaxReadBytesPerPoll = 64 * 1024;
	constexpr auto        kDisplayPrefix = "ConsoleLink:";

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

	bool TryGetFileIdentity(HANDLE handle, Display::OutboxFileIdentity& identity, DWORD& errorCode) noexcept
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

	std::string BuildConsoleLine(std::string_view line)
	{
		return std::string(kDisplayPrefix) + " " + std::string(line);
	}

	std::string_view DescribeResetReason(const Display::OutboxResetReason reason) noexcept
	{
		switch (reason) {
		case Display::OutboxResetReason::kRemoved:
			return "was removed";
		case Display::OutboxResetReason::kTruncated:
			return "was truncated";
		case Display::OutboxResetReason::kReplaced:
			return "was atomically replaced";
		case Display::OutboxResetReason::kNone:
		default:
			return "";
		}
	}

	class DisplayService
	{
	public:
		static DisplayService& GetSingleton()
		{
			static DisplayService service;
			return service;
		}

		void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath)
		{
			auto expected = false;
			if (!_initialized.compare_exchange_strong(expected, true)) {
				return;
			}

			if (!REL::Module::IsVR()) {
				logs::warn("Skipping console display: the first implementation slice is VR-only");
				return;
			}

			_settings = settings;
			_displayEnabled = settings.enableConsoleDisplay;

			if (!_displayEnabled) {
				logs::info("Console display is disabled; outbox service will remain dormant");
				return;
			}

			_outboxPath = outboxPath;

			const auto* messaging = SKSE::GetMessagingInterface();
			if (messaging == nullptr) {
				logs::warn("Skipping console display: SKSE messaging interface is unavailable");
				return;
			}

			if (!messaging->RegisterListener(MessageHandler)) {
				logs::warn("Skipping console display: failed to register the SKSE messaging listener");
				return;
			}

			_listenerRegistered = true;
			logs::info("Console display watching '{}'", PathToString(_outboxPath));
		}

		Display::ImmediatePrintResult TryPrintImmediateLine(std::string_view line) noexcept
		{
			if (!_displayEnabled) {
				return Display::ImmediatePrintResult::kDisabled;
			}

			try {
				auto* consoleLog = RE::ConsoleLog::GetSingleton();
				if (consoleLog == nullptr) {
					return Display::ImmediatePrintResult::kUnavailable;
				}

				const auto consoleLine = BuildConsoleLine(line);
				const Console::SuppressOutputScope suppressConsoleOutput;
				consoleLog->Print("%s", consoleLine.c_str());
				return Display::ImmediatePrintResult::kPrinted;
			} catch (const std::exception& exception) {
				try {
					logs::warn("Direct console display failed: {}", exception.what());
				} catch (...) {
					// Intentionally swallow logger failures so direct display remains fail-soft.
				}
			} catch (...) {
				try {
					logs::warn("Direct console display failed");
				} catch (...) {
					// Intentionally swallow logger failures so direct display remains fail-soft.
				}
			}

			return Display::ImmediatePrintResult::kUnavailable;
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
				StartWorker();
			}
		}

		void StartWorker()
		{
			std::scoped_lock lock(_workerStartLock);
			if (!_listenerRegistered || _serviceDisabled.load() || _workerStarted) {
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
				_workerStarted = true;
				logs::info("Starting console display worker thread");
			} catch (const std::exception& exception) {
				DisableService("worker startup failure", exception.what());
			} catch (...) {
				DisableService("worker startup failure", "unknown exception");
			}
		}

		void PollLoop(std::stop_token stopToken)
		{
			while (!stopToken.stop_requested() && !_serviceDisabled.load()) {
				PollOutbox();
				ScheduleFlushIfNeeded();
				std::this_thread::sleep_for(kPollInterval);
			}
		}

		void PollOutbox()
		{
			if (_serviceDisabled.load()) {
				return;
			}

			const auto handle = ScopedHandle(::CreateFileW(
				_outboxPath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr));

			if (handle.get() == INVALID_HANDLE_VALUE) {
				const auto error = ::GetLastError();
				if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
					Display::OutboxUpdateResult updateResult{};
					{
						std::scoped_lock lock(_queueLock);
						updateResult = _outboxState.ObserveMissingFile();
					}
					LogOutboxUpdate(updateResult);
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

			Display::OutboxFileIdentity identity{};
			DWORD                      identityError = ERROR_SUCCESS;
			if (!TryGetFileIdentity(handle.get(), identity, identityError)) {
				ReportFileError("identity", identityError);
				return;
			}

			ClearFileErrorState();

			auto                       cursor = std::uint64_t{};
			const auto                 size = static_cast<std::uint64_t>(fileSize.QuadPart);
			Display::OutboxUpdateResult observeResult{};
			{
				std::scoped_lock lock(_queueLock);
				observeResult = _outboxState.ObserveFile(identity, size);
				cursor = observeResult.readOffset;
			}
			LogOutboxUpdate(observeResult);

			if (!observeResult.shouldRead) {
				return;
			}

			const auto    bytesToRead = static_cast<DWORD>(std::min<std::uint64_t>(size - cursor, kMaxReadBytesPerPoll));
			LARGE_INTEGER offset{};
			offset.QuadPart = static_cast<LONGLONG>(cursor);
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

			Display::OutboxUpdateResult appendResult{};
			{
				std::scoped_lock lock(_queueLock);
				appendResult = _outboxState.AppendChunk(chunk);
			}
			LogOutboxUpdate(appendResult);
		}

		void LogOutboxUpdate(const Display::OutboxUpdateResult& result)
		{
			if (result.resetReason != Display::OutboxResetReason::kNone) {
				if (result.droppedPendingLines > 0) {
					logs::info(
						"Display outbox file '{}' {}; cleared {} queued display line(s) and reset the read cursor",
						PathToString(_outboxPath),
						DescribeResetReason(result.resetReason),
						result.droppedPendingLines);
				} else {
					logs::info(
						"Display outbox file '{}' {}; reset the read cursor",
						PathToString(_outboxPath),
						DescribeResetReason(result.resetReason));
				}
			}

			if (result.extractedCount > 0) {
				logs::info(
					"Queued {} display line(s) from '{}'; pending={}",
					result.extractedCount,
					PathToString(_outboxPath),
					result.pendingCount);
			}

			if (result.queueAtCapacity) {
				if (!_loggedPendingQueueFull) {
					logs::warn(
						"Pausing display outbox ingestion for '{}' because the pending display queue reached {}; buffered lines will wait until the queue drains",
						PathToString(_outboxPath),
						_outboxState.MaxPendingLines());
					_loggedPendingQueueFull = true;
				}
			} else {
				_loggedPendingQueueFull = false;
			}

			if (result.oversizedFragmentBytes > 0) {
				if (!_loggedOversizedFragment) {
					logs::warn(
						"Discarding an oversized partial display line from '{}' after {} bytes without a newline",
						PathToString(_outboxPath),
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

			if (!HasPendingLines()) {
				return;
			}

			if (_flushScheduled.exchange(true)) {
				return;
			}

			const auto* taskInterface = SKSE::GetTaskInterface();
			if (taskInterface == nullptr) {
				_flushScheduled = false;
				if (!_loggedMissingTaskInterface.exchange(true)) {
					logs::warn("Console display is waiting for the SKSE task interface");
				}
				return;
			}

			_loggedMissingTaskInterface = false;
			try {
				taskInterface->AddTask([] {
					try {
						DisplayService::GetSingleton().FlushOnGameThread();
					} catch (const std::exception& exception) {
						DisplayService::GetSingleton().DisableService("game-thread flush failure", exception.what());
					} catch (...) {
						DisplayService::GetSingleton().DisableService("game-thread flush failure", "unknown exception");
					}
				});
			} catch (const std::exception& exception) {
				DisableService("task scheduling failure", exception.what());
			} catch (...) {
				DisableService("task scheduling failure", "unknown exception");
			}
		}

		bool HasPendingLines()
		{
			std::scoped_lock lock(_queueLock);
			return _outboxState.HasPendingLines();
		}

		void FlushOnGameThread()
		{
			if (_serviceDisabled.load()) {
				_flushScheduled = false;
				return;
			}

			auto* consoleLog = RE::ConsoleLog::GetSingleton();
			if (consoleLog == nullptr) {
				if (!_loggedConsoleUnavailable) {
					logs::warn("Console display is holding pending lines because RE::ConsoleLog is not available yet");
					_loggedConsoleUnavailable = true;
				}
				_flushScheduled = false;
				return;
			}

			_loggedConsoleUnavailable = false;

			std::vector<std::string> lines;
			{
				std::scoped_lock lock(_queueLock);
				const auto       lineBudget = std::max<std::uint32_t>(_settings.maxDisplayLinesPerTick, 1);
				lines = _outboxState.DrainLines(lineBudget);
			}

			if (!lines.empty()) {
				logs::info("Flushing {} console display line(s)", lines.size());
			}

			for (const auto& line : lines) {
				const auto consoleLine = BuildConsoleLine(line);
				const Console::SuppressOutputScope suppressConsoleOutput;
				consoleLog->Print("%s", consoleLine.c_str());
				Logging::LogDisplayLine(consoleLine);
			}

			_flushScheduled = false;
			ScheduleFlushIfNeeded();
		}

		void ReportFileError(std::string_view operation, DWORD errorCode)
		{
			if (_lastFileErrorCode == errorCode && _lastFileErrorOperation == operation) {
				return;
			}

			_lastFileErrorCode = errorCode;
			_lastFileErrorOperation = std::string(operation);
			logs::warn(
				"Console display {} failed for '{}': win32={} {}",
				operation,
				PathToString(_outboxPath),
				errorCode,
				FormatWindowsError(errorCode));
		}

		void ClearFileErrorState()
		{
			_lastFileErrorCode = ERROR_SUCCESS;
			_lastFileErrorOperation.clear();
		}

		void DisableService(std::string_view context, std::string_view reason) noexcept
		{
			_serviceDisabled = true;
			_flushScheduled = false;

			if (_failureLogged.exchange(true)) {
				return;
			}

			try {
				logs::warn("Disabling console display service after {}: {}", context, reason);
			} catch (...) {
				// Intentionally swallow logger failures so async boundaries remain fail-soft.
			}
		}

		Config::Settings        _settings{};
		std::filesystem::path   _outboxPath;
		std::jthread            _worker;
		std::mutex              _workerStartLock;
		std::mutex              _queueLock;
		Display::OutboxState    _outboxState;
		std::atomic_bool        _initialized = false;
		bool                    _displayEnabled = false;
		std::atomic_bool        _serviceDisabled = false;
		std::atomic_bool        _failureLogged = false;
		std::atomic_bool        _flushScheduled = false;
		bool                    _workerStarted = false;
		bool                    _listenerRegistered = false;
		bool                    _loggedConsoleUnavailable = false;
		std::atomic_bool        _loggedMissingTaskInterface = false;
		bool                    _loggedPendingQueueFull = false;
		bool                    _loggedOversizedFragment = false;
		DWORD                   _lastFileErrorCode = ERROR_SUCCESS;
		std::string             _lastFileErrorOperation;
	};
}

namespace Display
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath)
	{
		DisplayService::GetSingleton().Initialize(settings, outboxPath);
	}

	ImmediatePrintResult TryPrintImmediateLine(std::string_view line) noexcept
	{
		return DisplayService::GetSingleton().TryPrintImmediateLine(line);
	}
}

