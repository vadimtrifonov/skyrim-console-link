#include "inbox/StartupInboxClear.h"

#include <Windows.h>

namespace
{
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
}

namespace Inbox
{
	constexpr std::string_view kStartupInboxClearFailureLine =
		"inbox startup failed: could not clear existing inbox contents";

	StartupClearResult ClearAtStartup(const std::filesystem::path& inboxPath) noexcept
	{
		const auto handle = ScopedHandle(::CreateFileW(
			inboxPath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr));

		if (handle.get() == INVALID_HANDLE_VALUE) {
			return { StartupClearResultKind::kFailedOpenOrCreate, ::GetLastError() };
		}

		BY_HANDLE_FILE_INFORMATION info{};
		if (!::GetFileInformationByHandle(handle.get(), &info)) {
			return { StartupClearResultKind::kFailedInspect, ::GetLastError() };
		}

		if (info.nFileSizeHigh != 0 || info.nFileSizeLow != 0) {
			LARGE_INTEGER zero{};
			if (!::SetFilePointerEx(handle.get(), zero, nullptr, FILE_BEGIN)) {
				return { StartupClearResultKind::kFailedClear, ::GetLastError() };
			}

			if (!::SetEndOfFile(handle.get())) {
				return { StartupClearResultKind::kFailedClear, ::GetLastError() };
			}
		}

		return {
			.identity = {
				.volumeSerialNumber = info.dwVolumeSerialNumber,
				.fileIndexHigh = info.nFileIndexHigh,
				.fileIndexLow = info.nFileIndexLow,
			},
		};
	}

	StartupClearAction DescribeStartupClearResult(const StartupClearResult result) noexcept
	{
		switch (result.kind) {
		case StartupClearResultKind::kEstablished:
			return {
				.shouldProceed = true,
				.shouldResetState = true,
				.localActivityLine = "inbox ready for new input",
			};
		case StartupClearResultKind::kFailedOpenOrCreate:
			return {
				.localActivityLine = kStartupInboxClearFailureLine,
				.disableReason = kStartupInboxClearFailureLine,
				.failedOperation = "open-or-create",
			};
		case StartupClearResultKind::kFailedInspect:
			return {
				.localActivityLine = kStartupInboxClearFailureLine,
				.disableReason = kStartupInboxClearFailureLine,
				.failedOperation = "observe",
			};
		case StartupClearResultKind::kFailedClear:
			return {
				.localActivityLine = kStartupInboxClearFailureLine,
				.disableReason = kStartupInboxClearFailureLine,
				.failedOperation = "clear",
			};
		default:
			return {
				.localActivityLine = kStartupInboxClearFailureLine,
				.disableReason = kStartupInboxClearFailureLine,
				.failedOperation = "unknown",
			};
		}
	}
}
