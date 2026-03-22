#include "config/Settings.h"
#include "command/InboxPath.h"
#include "display/OutboxPath.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

void RunCommandSubmissionStateTests();
void RunDisplayStateTests();

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
		path /= "ConsoleLink-tests-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(counter++) + ".ini";
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
		Expect(result.settings.enableConsoleDisplay == defaults.enableConsoleDisplay, "default display should be enabled");
		Expect(result.settings.enableConsoleCommandSubmission == defaults.enableConsoleCommandSubmission, "default command submission mismatch");
		Expect(result.settings.mirrorToSeparateConsoleLog == defaults.mirrorToSeparateConsoleLog, "default separate console log should be enabled");
		Expect(result.settings.includeTimestamps == defaults.includeTimestamps, "default timestamps should be enabled");
		Expect(result.settings.outboxPath == defaults.outboxPath, "default outbox path mismatch");
		Expect(result.settings.maxDisplayLinesPerTick == defaults.maxDisplayLinesPerTick, "default max display lines per tick mismatch");
		Expect(result.settings.inboxPath == defaults.inboxPath, "default inbox path mismatch");
		Expect(result.settings.maxCommandsPerTick == defaults.maxCommandsPerTick, "default max commands per tick mismatch");
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
			"EnableConsoleDisplay = off\n"
			"EnableConsoleCommandSubmission = true\n"
			"\n"
			"[Logging]\n"
			"MirrorToSeparateConsoleLog = false\n"
			"IncludeTimestamps = 0\n"
			"\n"
			"[Display]\n"
			"OutboxPath = Alt.outbox.txt\n"
			"MaxLinesPerTick = 3\n"
			"\n"
			"[CommandSubmission]\n"
			"InboxPath = Alt.inbox.txt\n"
			"MaxCommandsPerTick = 2\n"
			"\n"
			"[Debug]\n"
			"TraceHookInstall = true\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleInputLogging, "override input logging mismatch");
		Expect(!result.settings.enableConsoleOutputLogging, "override output logging mismatch");
		Expect(!result.settings.enableConsoleDisplay, "override display mismatch");
		Expect(result.settings.enableConsoleCommandSubmission, "override command submission mismatch");
		Expect(!result.settings.mirrorToSeparateConsoleLog, "override separate console log mismatch");
		Expect(!result.settings.includeTimestamps, "override timestamps mismatch");
		Expect(result.settings.outboxPath == std::filesystem::path(L"Alt.outbox.txt"), "override outbox path mismatch");
		Expect(result.settings.maxDisplayLinesPerTick == 3, "override max display lines per tick mismatch");
		Expect(result.settings.inboxPath == std::filesystem::path(L"Alt.inbox.txt"), "override inbox path mismatch");
		Expect(result.settings.maxCommandsPerTick == 2, "override max commands per tick mismatch");
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
			"[Display]\n"
			"MaxLinesPerTick = nope\n"
			"\n"
			"[CommandSubmission]\n"
			"MaxCommandsPerTick = invalid\n"
			"\n"
			"[Debug]\n"
			"TraceHookInstall = perhaps\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsoleInputLogging, "malformed bool should fall back to default");
		Expect(result.settings.maxDisplayLinesPerTick == 8, "malformed uint should fall back to default");
		Expect(result.settings.maxCommandsPerTick == 4, "malformed command uint should fall back to default");
		Expect(!result.settings.traceHookInstall, "malformed trace flag should fall back to default");
		Expect(result.warnings.size() == 4, "expected malformed warnings");

		std::filesystem::remove(path);
	}

	void TestMissingOutboxPathUsesDefault()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Display]\n"
			"MaxLinesPerTick = 4\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsoleDisplay, "missing outbox path should not disable display");
		Expect(result.settings.outboxPath == Config::GetDefaultOutboxPath(), "missing outbox path should use the default");
		Expect(result.warnings.empty(), "missing outbox path should not produce warnings");

		std::filesystem::remove(path);
	}

	void TestEmptyOutboxPathDisablesDisplay()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Display]\n"
			"OutboxPath =   \n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleDisplay, "empty outbox path should disable display");
		Expect(result.settings.outboxPath.empty(), "empty outbox path should not keep a usable path");
		Expect(result.warnings.size() == 1, "expected one warning for empty outbox path");

		std::filesystem::remove(path);
	}

	void TestEmptyInboxPathDisablesCommandSubmission()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Main]\n"
			"EnableConsoleCommandSubmission = true\n"
			"\n"
			"[CommandSubmission]\n"
			"InboxPath =   \n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleCommandSubmission, "empty inbox path should disable command submission");
		Expect(result.settings.inboxPath.empty(), "empty inbox path should not keep a usable path");
		Expect(result.warnings.size() == 1, "expected one warning for empty inbox path");

		std::filesystem::remove(path);
	}

	void TestOversizedOutboxPathDisablesDisplay()
	{
		const auto path = MakeTempIniPath();
		const auto oversizedPath = std::string(600, 'A');
		WriteFile(
			path,
			"[Display]\n"
			"OutboxPath = " +
				oversizedPath + "\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleDisplay, "oversized outbox path should disable display");
		Expect(result.settings.outboxPath.empty(), "oversized outbox path should not keep a usable path");
		Expect(result.warnings.size() == 1, "expected one warning for oversized outbox path");

		std::filesystem::remove(path);
	}

	void TestOversizedInboxPathDisablesCommandSubmission()
	{
		const auto path = MakeTempIniPath();
		const auto oversizedPath = std::string(600, 'B');
		WriteFile(
			path,
			"[Main]\n"
			"EnableConsoleCommandSubmission = true\n"
			"\n"
			"[CommandSubmission]\n"
			"InboxPath = " +
				oversizedPath + "\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleCommandSubmission, "oversized inbox path should disable command submission");
		Expect(result.settings.inboxPath.empty(), "oversized inbox path should not keep a usable path");
		Expect(result.warnings.size() == 1, "expected one warning for oversized inbox path");

		std::filesystem::remove(path);
	}

	void TestLegacyOutboxPathRemapsToLogDirectory()
	{
		const std::filesystem::path configured = L"Data/SKSE/Plugins/ConsoleLink.outbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleLink-log-root";
		const auto                  result = Display::ResolveOutboxPath(configured, logDirectory);

		Expect(result.configuredPath == configured, "legacy configured outbox path mismatch");
		Expect(result.remappedLegacyDefault, "legacy outbox path should be remapped");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / L"ConsoleLink.outbox.txt"), "legacy outbox path should resolve under the log directory");
	}

	void TestRelativeOutboxPathResolvesUnderLogDirectory()
	{
		const std::filesystem::path configured = L"ConsoleLink.outbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleLink-log-root";
		const auto                  result = Display::ResolveOutboxPath(configured, logDirectory);

		Expect(result.configuredPath == configured, "relative configured outbox path mismatch");
		Expect(!result.remappedLegacyDefault, "plain relative outbox path should not be remapped");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / configured), "relative outbox path should resolve under the log directory");
	}

	void TestRelativeInboxPathResolvesUnderLogDirectory()
	{
		const std::filesystem::path configured = L"ConsoleLink.inbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleLink-log-root";
		const auto                  result = Command::ResolveInboxPath(configured, logDirectory);

		Expect(result.configuredPath == configured, "configured inbox path mismatch");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / configured), "relative inbox path should resolve under the log directory");
	}
}

int main()
{
	try {
		TestDefaults();
		TestOverrides();
		TestMalformedFallbacks();
		TestMissingOutboxPathUsesDefault();
		TestEmptyOutboxPathDisablesDisplay();
		TestEmptyInboxPathDisablesCommandSubmission();
		TestOversizedOutboxPathDisablesDisplay();
		TestOversizedInboxPathDisablesCommandSubmission();
		TestLegacyOutboxPathRemapsToLogDirectory();
		TestRelativeOutboxPathResolvesUnderLogDirectory();
		TestRelativeInboxPathResolvesUnderLogDirectory();
		RunCommandSubmissionStateTests();
		RunDisplayStateTests();
		std::cout << "All tests passed.\n";
		return 0;
	} catch (const std::exception& exception) {
		std::cerr << "Tests failed: " << exception.what() << '\n';
		return 1;
	}
}

