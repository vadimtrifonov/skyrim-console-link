#include "writeback/WriteBackService.h"

// clang-format off
#include "pch.h"
#include "RE/C/ConsoleLog.h"
#include "logging/ConsoleActivityLog.h"
// clang-format on

#include <Windows.h>
#include <cstdarg>

namespace
{
	using namespace std::chrono_literals;

	constexpr auto        kPollInterval = 200ms;
	constexpr std::size_t kMaxReadBytesPerPoll = 64 * 1024;
	constexpr std::size_t kMaxPendingFragmentBytes = 8 * 1024;
	constexpr auto        kWriteBackPrefix = "ConsoleDebugger:";

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

	class WriteBackService
	{
	public:
		static WriteBackService& GetSingleton()
		{
			static WriteBackService service;
			return service;
		}

		void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath)
		{
			auto expected = false;
			if (!_initialized.compare_exchange_strong(expected, true)) {
				return;
			}

			if (!REL::Module::IsVR()) {
				logs::warn("Skipping console write-back: the first implementation slice is VR-only");
				return;
			}

			if (!settings.enableConsoleWriteBack) {
				logs::info("Console write-back is disabled; outbox service will remain dormant");
				return;
			}

			_settings = settings;
			_outboxPath = outboxPath;

			const auto* messaging = SKSE::GetMessagingInterface();
			if (messaging == nullptr) {
				logs::warn("Skipping console write-back: SKSE messaging interface is unavailable");
				return;
			}

			if (!messaging->RegisterListener(MessageHandler)) {
				logs::warn("Skipping console write-back: failed to register the SKSE messaging listener");
				return;
			}

			_listenerRegistered = true;
			logs::info("Console write-back watching '{}'", PathToString(_outboxPath));
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
			if (!_listenerRegistered || _workerStarted.exchange(true)) {
				return;
			}

			logs::info("Starting console write-back worker thread");
			_worker = std::jthread([this](std::stop_token stopToken) {
				PollLoop(stopToken);
			});
		}

		void PollLoop(std::stop_token stopToken)
		{
			while (!stopToken.stop_requested()) {
				PollOutbox();
				ScheduleFlushIfNeeded();
				std::this_thread::sleep_for(kPollInterval);
			}
		}

		void PollOutbox()
		{
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

			ClearFileErrorState();

			auto       cursor = std::uint64_t{};
			const auto size = static_cast<std::uint64_t>(fileSize.QuadPart);
			{
				std::scoped_lock lock(_queueLock);
				if (size < _fileCursor) {
					_fileCursor = 0;
					_pendingFragment.clear();
					logs::info("Outbox file '{}' was truncated or replaced; resetting the read cursor", PathToString(_outboxPath));
				}

				cursor = _fileCursor;
			}

			if (size <= cursor) {
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

			{
				std::scoped_lock lock(_queueLock);
				_fileCursor = cursor + bytesRead;
				_pendingFragment.append(chunk);
				ExtractCompleteLinesLocked();
			}
		}

		void ExtractCompleteLinesLocked()
		{
			std::size_t extractedCount = 0;
			while (true) {
				const auto newline = _pendingFragment.find('\n');
				if (newline == std::string::npos) {
					break;
				}

				auto line = _pendingFragment.substr(0, newline);
				_pendingFragment.erase(0, newline + 1);

				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				if (!line.empty()) {
					_pendingLines.push_back(std::move(line));
					++extractedCount;
				}
			}

			if (extractedCount > 0) {
				logs::info(
					"Queued {} outbox line(s) from '{}'; pending={}",
					extractedCount,
					PathToString(_outboxPath),
					_pendingLines.size());
			}

			if (_pendingFragment.size() > kMaxPendingFragmentBytes) {
				if (!_loggedOversizedFragment) {
					logs::warn(
						"Discarding an oversized partial outbox line from '{}' after {} bytes without a newline",
						PathToString(_outboxPath),
						_pendingFragment.size());
					_loggedOversizedFragment = true;
				}
				_pendingFragment.clear();
			} else if (_pendingFragment.empty()) {
				_loggedOversizedFragment = false;
			}
		}

		void ScheduleFlushIfNeeded()
		{
			if (!HasPendingLines()) {
				return;
			}

			if (_flushScheduled.exchange(true)) {
				return;
			}

			const auto* taskInterface = SKSE::GetTaskInterface();
			if (taskInterface == nullptr) {
				_flushScheduled = false;
				if (!_loggedMissingTaskInterface) {
					logs::warn("Console write-back is waiting for the SKSE task interface");
					_loggedMissingTaskInterface = true;
				}
				return;
			}

			_loggedMissingTaskInterface = false;
			taskInterface->AddTask([] {
				WriteBackService::GetSingleton().FlushOnGameThread();
			});
		}

		bool HasPendingLines()
		{
			std::scoped_lock lock(_queueLock);
			return !_pendingLines.empty();
		}

		void FlushOnGameThread()
		{
			auto* consoleLog = RE::ConsoleLog::GetSingleton();
			if (consoleLog == nullptr) {
				if (!_loggedConsoleUnavailable) {
					logs::warn("Console write-back is holding pending lines because RE::ConsoleLog is not available yet");
					_loggedConsoleUnavailable = true;
				}
				_flushScheduled = false;
				return;
			}

			_loggedConsoleUnavailable = false;

			std::vector<std::string> lines;
			{
				std::scoped_lock lock(_queueLock);
				const auto       lineBudget = std::max<std::uint32_t>(_settings.maxLinesPerTick, 1);
				const auto       lineCount = std::min<std::size_t>(_pendingLines.size(), lineBudget);
				lines.reserve(lineCount);
				for (std::size_t index = 0; index < lineCount; ++index) {
					lines.push_back(std::move(_pendingLines.front()));
					_pendingLines.pop_front();
				}
			}

			if (!lines.empty()) {
				logs::info("Flushing {} console write-back line(s)", lines.size());
			}

			for (const auto& line : lines) {
				const auto consoleLine = std::string(kWriteBackPrefix) + " " + line;
				consoleLog->Print("%s", consoleLine.c_str());
				Logging::LogWriteBackLine(consoleLine);
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
				"Console write-back {} failed for '{}': win32={} {}",
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

		Config::Settings        _settings{};
		std::filesystem::path   _outboxPath;
		std::jthread            _worker;
		std::mutex              _queueLock;
		std::deque<std::string> _pendingLines;
		std::string             _pendingFragment;
		std::uint64_t           _fileCursor = 0;
		std::atomic_bool        _initialized = false;
		std::atomic_bool        _workerStarted = false;
		std::atomic_bool        _flushScheduled = false;
		bool                    _listenerRegistered = false;
		bool                    _loggedConsoleUnavailable = false;
		bool                    _loggedMissingTaskInterface = false;
		bool                    _loggedOversizedFragment = false;
		DWORD                   _lastFileErrorCode = ERROR_SUCCESS;
		std::string             _lastFileErrorOperation;
	};
}

namespace WriteBack
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath)
	{
		WriteBackService::GetSingleton().Initialize(settings, outboxPath);
	}
}
