#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Inbox
{
	enum class StartupClearResultKind : std::uint8_t
	{
		kEstablished,
		kFailedOpenOrCreate,
		kFailedInspect,
		kFailedClear
	};

	struct StartupClearResult
	{
		StartupClearResultKind kind = StartupClearResultKind::kEstablished;
		std::uint32_t errorCode = 0;
	};

	struct StartupClearAction
	{
		bool shouldProceed = false;
		bool shouldResetState = false;
		std::string_view localActivityLine;
		std::string_view disableReason;
		std::string_view failedOperation;
	};

	[[nodiscard]] StartupClearResult ClearAtStartup(const std::filesystem::path& inboxPath) noexcept;
	[[nodiscard]] StartupClearAction DescribeStartupClearResult(StartupClearResult result) noexcept;
}
