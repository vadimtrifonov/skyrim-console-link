#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Settings
{
	struct Values
	{
		bool enableConsoleInputLogging = true;
		bool enableConsoleOutputLogging = true;
		bool enableInbox = true;

		std::filesystem::path inboxPath = L"ConsoleLink.inbox.txt";
		std::uint32_t maxInboxLinesPerTick = 1;
	};

	struct LoadResult
	{
		Values settings;
		std::vector<std::string> warnings;
	};

	[[nodiscard]] std::filesystem::path GetDefaultSettingsPath();
	[[nodiscard]] std::filesystem::path GetDefaultInboxPath();
	[[nodiscard]] Values GetBuiltInDefaults();
	[[nodiscard]] LoadResult LoadSettingsWithDiagnostics(const std::filesystem::path& path);
	[[nodiscard]] Values LoadSettings(const std::filesystem::path& path);
	[[nodiscard]] Values LoadDefaultSettings();
}
