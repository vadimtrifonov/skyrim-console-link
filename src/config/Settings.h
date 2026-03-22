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
		bool enableConsoleWriteBack = true;

		bool mirrorToSeparateConsoleLog = true;
		bool includeTimestamps = true;

		std::filesystem::path outboxPath = L"Data/SKSE/Plugins/ConsoleDebugger.outbox.txt";
		std::uint32_t         maxLinesPerTick = 8;

		bool traceHookInstall = false;
	};

	struct LoadResult
	{
		Settings                 settings;
		std::vector<std::string> warnings;
	};

	[[nodiscard]] std::filesystem::path GetDefaultSettingsPath();
	[[nodiscard]] LoadResult            LoadSettingsWithDiagnostics(const std::filesystem::path& path);
	[[nodiscard]] Settings              LoadSettings(const std::filesystem::path& path);
	[[nodiscard]] Settings              LoadDefaultSettings();
}
