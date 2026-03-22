#include "config/Settings.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	std::filesystem::path MakeTempIniPath()
	{
		static std::uint32_t counter = 0;
		auto                 path = std::filesystem::temp_directory_path();
		path /= "ConsoleDebugger-tests-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(counter++) + ".ini";
		return path;
	}

	void WriteFile(const std::filesystem::path& path, std::string_view contents)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		stream << contents;
	}

	void TestDefaults()
	{
		const auto path = MakeTempIniPath();
		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsoleInputLogging, "default input logging should be enabled");
		Expect(result.settings.enableConsoleOutputLogging, "default output logging should be enabled");
		Expect(result.settings.enableConsoleWriteBack, "default write-back should be enabled");
		Expect(result.settings.mirrorToSeparateConsoleLog, "default separate console log should be enabled");
		Expect(result.settings.includeTimestamps, "default timestamps should be enabled");
		Expect(result.settings.outboxPath == std::filesystem::path(L"Data/SKSE/Plugins/ConsoleDebugger.outbox.txt"), "default outbox path mismatch");
		Expect(result.settings.maxLinesPerTick == 8, "default max lines per tick mismatch");
		Expect(!result.settings.traceHookInstall, "default hook tracing should be disabled");
		Expect(result.warnings.empty(), "missing config should not produce warnings");
	}

	void TestOverrides()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Main]\n"
			"EnableConsoleInputLogging = false\n"
			"EnableConsoleOutputLogging = no\n"
			"EnableConsoleWriteBack = off\n"
			"\n"
			"[Logging]\n"
			"MirrorToSeparateConsoleLog = false\n"
			"IncludeTimestamps = 0\n"
			"\n"
			"[WriteBack]\n"
			"OutboxPath = Data/SKSE/Plugins/Alt.outbox.txt\n"
			"MaxLinesPerTick = 3\n"
			"\n"
			"[Debug]\n"
			"TraceHookInstall = true\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleInputLogging, "override input logging mismatch");
		Expect(!result.settings.enableConsoleOutputLogging, "override output logging mismatch");
		Expect(!result.settings.enableConsoleWriteBack, "override write-back mismatch");
		Expect(!result.settings.mirrorToSeparateConsoleLog, "override separate console log mismatch");
		Expect(!result.settings.includeTimestamps, "override timestamps mismatch");
		Expect(result.settings.outboxPath == std::filesystem::path(L"Data/SKSE/Plugins/Alt.outbox.txt"), "override outbox path mismatch");
		Expect(result.settings.maxLinesPerTick == 3, "override max lines per tick mismatch");
		Expect(result.settings.traceHookInstall, "override hook tracing mismatch");
		Expect(result.warnings.empty(), "valid config should not produce warnings");

		std::filesystem::remove(path);
	}

	void TestMalformedFallbacks()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Main]\n"
			"EnableConsoleInputLogging = maybe\n"
			"\n"
			"[WriteBack]\n"
			"MaxLinesPerTick = nope\n"
			"\n"
			"[Debug]\n"
			"TraceHookInstall = perhaps\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsoleInputLogging, "malformed bool should fall back to default");
		Expect(result.settings.maxLinesPerTick == 8, "malformed uint should fall back to default");
		Expect(!result.settings.traceHookInstall, "malformed trace flag should fall back to default");
		Expect(result.warnings.size() == 3, "expected malformed warnings");

		std::filesystem::remove(path);
	}
}

int main()
{
	try {
		TestDefaults();
		TestOverrides();
		TestMalformedFallbacks();
		std::cout << "All config tests passed.\n";
		return 0;
	} catch (const std::exception& exception) {
		std::cerr << "Config tests failed: " << exception.what() << '\n';
		return 1;
	}
}
