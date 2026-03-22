#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Config
{
	struct Settings
	{
		bool enableConsoleInputLogging = true;
		bool enableConsoleOutputLogging = true;
		bool enableConsoleDisplay = true;
		bool enableConsoleCommandSubmission = true;

		bool includeTimestamps = true;

		std::filesystem::path outboxPath = L"ConsoleLink.outbox.txt";
		std::uint32_t         maxDisplayLinesPerTick = 8;
		std::filesystem::path inboxPath = L"ConsoleLink.inbox.txt";
		std::uint32_t         maxCommandsPerTick = 4;

		bool traceHookInstall = false;
	};

	struct LoadResult
	{
		Settings                 settings;
		std::vector<std::string> warnings;
	};

	[[nodiscard]] std::filesystem::path GetDefaultSettingsPath();
	[[nodiscard]] std::filesystem::path GetDefaultOutboxPath();
	[[nodiscard]] std::filesystem::path GetDefaultInboxPath();
	[[nodiscard]] Settings              GetBuiltInDefaults();
	[[nodiscard]] LoadResult            LoadSettingsWithDiagnostics(const std::filesystem::path& path);
	[[nodiscard]] Settings              LoadSettings(const std::filesystem::path& path);
	[[nodiscard]] Settings              LoadDefaultSettings();
}

