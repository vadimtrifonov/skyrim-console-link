#include "config/Settings.h"
#include "writeback/OutboxPath.h"

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
		const auto defaults = Config::GetBuiltInDefaults();

		Expect(result.settings.enableConsoleInputLogging == defaults.enableConsoleInputLogging, "default input logging should be enabled");
		Expect(result.settings.enableConsoleOutputLogging == defaults.enableConsoleOutputLogging, "default output logging should be enabled");
		Expect(result.settings.enableConsoleWriteBack == defaults.enableConsoleWriteBack, "default write-back should be enabled");
		Expect(result.settings.mirrorToSeparateConsoleLog == defaults.mirrorToSeparateConsoleLog, "default separate console log should be enabled");
		Expect(result.settings.includeTimestamps == defaults.includeTimestamps, "default timestamps should be enabled");
		Expect(result.settings.outboxPath == defaults.outboxPath, "default outbox path mismatch");
		Expect(result.settings.maxLinesPerTick == defaults.maxLinesPerTick, "default max lines per tick mismatch");
		Expect(result.settings.traceHookInstall == defaults.traceHookInstall, "default hook tracing should be disabled");
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
			"OutboxPath = Alt.outbox.txt\n"
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
		Expect(result.settings.outboxPath == std::filesystem::path(L"Alt.outbox.txt"), "override outbox path mismatch");
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

	void TestMissingOutboxPathUsesDefault()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[WriteBack]\n"
			"MaxLinesPerTick = 4\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsoleWriteBack, "missing outbox path should not disable write-back");
		Expect(result.settings.outboxPath == Config::GetDefaultOutboxPath(), "missing outbox path should use the default");
		Expect(result.warnings.empty(), "missing outbox path should not produce warnings");

		std::filesystem::remove(path);
	}

	void TestEmptyOutboxPathDisablesWriteBack()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[WriteBack]\n"
			"OutboxPath =   \n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleWriteBack, "empty outbox path should disable write-back");
		Expect(result.settings.outboxPath.empty(), "empty outbox path should not keep a usable path");
		Expect(result.warnings.size() == 1, "expected one warning for empty outbox path");

		std::filesystem::remove(path);
	}

	void TestOversizedOutboxPathDisablesWriteBack()
	{
		const auto path = MakeTempIniPath();
		const auto oversizedPath = std::string(600, 'A');
		WriteFile(
			path,
			"[WriteBack]\n"
			"OutboxPath = " +
				oversizedPath + "\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleWriteBack, "oversized outbox path should disable write-back");
		Expect(result.settings.outboxPath.empty(), "oversized outbox path should not keep a usable path");
		Expect(result.warnings.size() == 1, "expected one warning for oversized outbox path");

		std::filesystem::remove(path);
	}

	void TestLegacyOutboxPathRemapsToLogDirectory()
	{
		const std::filesystem::path configured = L"Data/SKSE/Plugins/ConsoleDebugger.outbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleDebugger-log-root";
		const auto                  result = WriteBack::ResolveOutboxPath(configured, logDirectory);

		Expect(result.configuredPath == configured, "legacy configured outbox path mismatch");
		Expect(result.remappedLegacyDefault, "legacy outbox path should be remapped");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / L"ConsoleDebugger.outbox.txt"), "legacy outbox path should resolve under the log directory");
	}

	void TestRelativeOutboxPathResolvesUnderLogDirectory()
	{
		const std::filesystem::path configured = L"ConsoleDebugger.outbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleDebugger-log-root";
		const auto                  result = WriteBack::ResolveOutboxPath(configured, logDirectory);

		Expect(result.configuredPath == configured, "relative configured outbox path mismatch");
		Expect(!result.remappedLegacyDefault, "plain relative outbox path should not be remapped");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / configured), "relative outbox path should resolve under the log directory");
	}
}

int main()
{
	try {
		TestDefaults();
		TestOverrides();
		TestMalformedFallbacks();
		TestMissingOutboxPathUsesDefault();
		TestEmptyOutboxPathDisablesWriteBack();
		TestOversizedOutboxPathDisablesWriteBack();
		TestLegacyOutboxPathRemapsToLogDirectory();
		TestRelativeOutboxPathResolvesUnderLogDirectory();
		std::cout << "All config tests passed.\n";
		return 0;
	} catch (const std::exception& exception) {
		std::cerr << "Config tests failed: " << exception.what() << '\n';
		return 1;
	}
}
