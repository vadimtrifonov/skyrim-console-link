#include "command/InboxPath.h"
#include "config/Settings.h"
#include "print/OutboxPath.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

void RunCommandSubmissionStateTests();
void RunConsoleActivityTaxonomyTests();
void RunPrintStateTests();
void RunLookupCommandTests();

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
		Expect(result.settings.enableConsolePrint == defaults.enableConsolePrint, "default print should be enabled");
		Expect(result.settings.enableConsoleCommandSubmission == defaults.enableConsoleCommandSubmission, "default command submission mismatch");
		Expect(result.settings.includeTimestamps == defaults.includeTimestamps, "default timestamps should be enabled");
		Expect(result.settings.outboxPath == defaults.outboxPath, "default outbox path mismatch");
		Expect(result.settings.maxPrintLinesPerTick == defaults.maxPrintLinesPerTick, "default max print lines per tick mismatch");
		Expect(result.settings.inboxPath == defaults.inboxPath, "default inbox path mismatch");
		Expect(result.settings.maxCommandsPerTick == defaults.maxCommandsPerTick, "default max commands per tick mismatch");
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
			"EnableConsolePrint = off\n"
			"EnableConsoleCommandSubmission = true\n"
			"\n"
			"[Logging]\n"
			"IncludeTimestamps = 0\n"
			"\n"
			"[Print]\n"
			"OutboxPath = Alt.outbox.txt\n"
			"MaxLinesPerTick = 3\n"
			"\n"
			"[CommandSubmission]\n"
			"InboxPath = Alt.inbox.txt\n"
			"MaxCommandsPerTick = 2\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsoleInputLogging, "override input logging mismatch");
		Expect(!result.settings.enableConsoleOutputLogging, "override output logging mismatch");
		Expect(!result.settings.enableConsolePrint, "override print mismatch");
		Expect(result.settings.enableConsoleCommandSubmission, "override command submission mismatch");
		Expect(!result.settings.includeTimestamps, "override timestamps mismatch");
		Expect(result.settings.outboxPath == std::filesystem::path(L"Alt.outbox.txt"), "override outbox path mismatch");
		Expect(result.settings.maxPrintLinesPerTick == 3, "override max print lines per tick mismatch");
		Expect(result.settings.inboxPath == std::filesystem::path(L"Alt.inbox.txt"), "override inbox path mismatch");
		Expect(result.settings.maxCommandsPerTick == 2, "override max commands per tick mismatch");
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
			"[Print]\n"
			"MaxLinesPerTick = nope\n"
			"\n"
			"[CommandSubmission]\n"
			"MaxCommandsPerTick = invalid\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsoleInputLogging, "malformed bool should fall back to default");
		Expect(result.settings.maxPrintLinesPerTick == 8, "malformed uint should fall back to default");
		Expect(result.settings.maxCommandsPerTick == 4, "malformed command uint should fall back to default");
		Expect(result.warnings.size() == 3, "expected malformed warnings");

		std::filesystem::remove(path);
	}

	void TestMissingOutboxPathUsesDefault()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Print]\n"
			"MaxLinesPerTick = 4\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(result.settings.enableConsolePrint, "missing outbox path should not disable print");
		Expect(result.settings.outboxPath == Config::GetDefaultOutboxPath(), "missing outbox path should use the default");
		Expect(result.warnings.empty(), "missing outbox path should not produce warnings");

		std::filesystem::remove(path);
	}

	void TestEmptyOutboxPathDisablesPrint()
	{
		const auto path = MakeTempIniPath();
		WriteFile(
			path,
			"[Print]\n"
			"OutboxPath =   \n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsolePrint, "empty outbox path should disable print");
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

	void TestOversizedOutboxPathDisablesPrint()
	{
		const auto path = MakeTempIniPath();
		const auto oversizedPath = std::string(600, 'A');
		WriteFile(
			path,
			"[Print]\n"
			"OutboxPath = " +
				oversizedPath + "\n");

		const auto result = Config::LoadSettingsWithDiagnostics(path);

		Expect(!result.settings.enableConsolePrint, "oversized outbox path should disable print");
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

	void TestRelativeOutboxPathResolvesUnderLogDirectory()
	{
		const std::filesystem::path configured = L"ConsoleLink.outbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleLink-log-root";
		const auto                  result = Print::ResolveOutboxPath(Print::OutboxPathContext{
							 .configuredPath = configured,
							 .logDirectory = logDirectory });

		Expect(result.configuredPath == configured, "relative configured outbox path mismatch");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / configured), "relative outbox path should resolve under the log directory");
	}

	void TestRelativeInboxPathResolvesUnderLogDirectory()
	{
		const std::filesystem::path configured = L"ConsoleLink.inbox.txt";
		const auto                  logDirectory = std::filesystem::temp_directory_path() / "ConsoleLink-log-root";
		const auto                  result = Command::ResolveInboxPath(Command::InboxPathContext{
							 .configuredPath = configured,
							 .logDirectory = logDirectory });

		Expect(result.configuredPath == configured, "configured inbox path mismatch");
		Expect(result.resolvedPath == std::filesystem::absolute(logDirectory / configured), "relative inbox path should resolve under the log directory");
	}
}

int main() noexcept  // NOLINT(bugprone-exception-escape)
{
	try {
		TestDefaults();
		TestOverrides();
		TestMalformedFallbacks();
		TestMissingOutboxPathUsesDefault();
		TestEmptyOutboxPathDisablesPrint();
		TestEmptyInboxPathDisablesCommandSubmission();
		TestOversizedOutboxPathDisablesPrint();
		TestOversizedInboxPathDisablesCommandSubmission();
		TestRelativeOutboxPathResolvesUnderLogDirectory();
		TestRelativeInboxPathResolvesUnderLogDirectory();
		RunCommandSubmissionStateTests();
		RunConsoleActivityTaxonomyTests();
		RunPrintStateTests();
		RunLookupCommandTests();
		std::cout << "All tests passed.\n";
		return 0;
	} catch (const std::exception& exception) {
		std::cerr << "Tests failed: " << exception.what() << '\n';
		return 1;
	} catch (...) {
		std::cerr << "Tests failed: unknown exception\n";
		return 1;
	}
}
