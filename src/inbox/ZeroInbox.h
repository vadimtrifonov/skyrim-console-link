#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Inbox
{
	enum class ZeroInboxResultKind : std::uint8_t
	{
		kEstablished,
		kFailedOpenOrCreate,
		kFailedInspect,
		kFailedClear
	};

	struct ZeroInboxResult
	{
		ZeroInboxResultKind kind = ZeroInboxResultKind::kEstablished;
		std::uint32_t errorCode = 0;
	};

	struct ZeroInboxAction
	{
		bool shouldProceed = false;
		bool shouldResetState = false;
		std::string_view localActivityLine;
		std::string_view disableReason;
		std::string_view failedOperation;
	};

	[[nodiscard]] ZeroInboxResult EstablishZeroInboxFile(const std::filesystem::path& inboxPath) noexcept;
	[[nodiscard]] ZeroInboxAction DescribeZeroInboxResult(ZeroInboxResult result) noexcept;
}
