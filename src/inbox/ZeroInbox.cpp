#include "inbox/ZeroInbox.h"

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
	constexpr std::string_view kZeroInboxFailureLine =
		"inbox startup failed: could not clear existing inbox contents";

	ZeroInboxResult EstablishZeroInboxFile(const std::filesystem::path& inboxPath) noexcept
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
			return { ZeroInboxResultKind::kFailedOpenOrCreate, ::GetLastError() };
		}

		LARGE_INTEGER fileSize{};
		if (!::GetFileSizeEx(handle.get(), &fileSize)) {
			return { ZeroInboxResultKind::kFailedInspect, ::GetLastError() };
		}

		if (fileSize.QuadPart != 0) {
			LARGE_INTEGER zero{};
			if (!::SetFilePointerEx(handle.get(), zero, nullptr, FILE_BEGIN)) {
				return { ZeroInboxResultKind::kFailedClear, ::GetLastError() };
			}

			if (!::SetEndOfFile(handle.get())) {
				return { ZeroInboxResultKind::kFailedClear, ::GetLastError() };
			}
		}

		return {};
	}

	ZeroInboxAction DescribeZeroInboxResult(const ZeroInboxResult result) noexcept
	{
		switch (result.kind) {
		case ZeroInboxResultKind::kEstablished:
			return {
				.shouldProceed = true,
				.shouldResetState = true,
				.localActivityLine = "inbox ready for new input",
			};
		case ZeroInboxResultKind::kFailedOpenOrCreate:
			return {
				.localActivityLine = kZeroInboxFailureLine,
				.disableReason = kZeroInboxFailureLine,
				.failedOperation = "open-or-create",
			};
		case ZeroInboxResultKind::kFailedInspect:
			return {
				.localActivityLine = kZeroInboxFailureLine,
				.disableReason = kZeroInboxFailureLine,
				.failedOperation = "size",
			};
		case ZeroInboxResultKind::kFailedClear:
			return {
				.localActivityLine = kZeroInboxFailureLine,
				.disableReason = kZeroInboxFailureLine,
				.failedOperation = "clear",
			};
		default:
			return {
				.localActivityLine = kZeroInboxFailureLine,
				.disableReason = kZeroInboxFailureLine,
				.failedOperation = "unknown",
			};
		}
	}
}
