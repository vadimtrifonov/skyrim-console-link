#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Inbox
{
	enum class StartupInboxClearResultKind : std::uint8_t
	{
		kEstablished,
		kFailedOpenOrCreate,
		kFailedInspect,
		kFailedClear
	};

	struct StartupInboxClearResult
	{
		StartupInboxClearResultKind kind = StartupInboxClearResultKind::kEstablished;
		std::uint32_t errorCode = 0;
	};

	struct StartupInboxClearAction
	{
		bool shouldProceed = false;
		bool shouldResetState = false;
		std::string_view localActivityLine;
		std::string_view disableReason;
		std::string_view failedOperation;
	};

	[[nodiscard]] StartupInboxClearResult ClearInboxAtStartup(const std::filesystem::path& inboxPath) noexcept;
	[[nodiscard]] StartupInboxClearAction DescribeStartupInboxClearResult(StartupInboxClearResult result) noexcept;
}
