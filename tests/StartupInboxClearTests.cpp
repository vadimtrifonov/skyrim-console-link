#include "inbox/StartupInboxClear.h"

#include <filesystem>
#include <fstream>
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

	std::filesystem::path MakeTempInboxPath()
	{
		static std::uint32_t counter = 0;
		auto path = std::filesystem::temp_directory_path();
		path /= "ConsoleLink-startup-inbox-clear-" + std::to_string(counter++) + ".txt";
		return path;
	}

	void WriteFile(const std::filesystem::path& path, std::string_view contents)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		stream << contents;
	}

	void TestSuccessActionResetsStateAndAnnouncesClear()
	{
		const auto action = Inbox::DescribeStartupClearResult({});
		Expect(action.shouldProceed, "successful startup inbox clear should proceed");
		Expect(action.shouldResetState, "successful startup inbox clear should reset inbox state");
		Expect(action.localActivityLine == "inbox ready for new input", "successful startup inbox clear should announce readiness");
		Expect(action.disableReason.empty(), "successful startup inbox clear should not disable inbox processing");
	}

	void TestFailureActionsDisableInboxProcessing()
	{
		const auto openFailure = Inbox::DescribeStartupClearResult({ .kind = Inbox::StartupClearResultKind::kFailedOpenOrCreate,
			.errorCode = 5 });
		Expect(!openFailure.shouldProceed, "open failure should stop startup inbox clear");
		Expect(!openFailure.shouldResetState, "open failure should not reset inbox state");
		Expect(
			openFailure.localActivityLine == "inbox startup failed: could not clear existing inbox contents",
			"open failure should announce startup inbox clear failure");
		Expect(
			openFailure.disableReason == "inbox startup failed: could not clear existing inbox contents",
			"open failure should disable inbox processing");
		Expect(openFailure.failedOperation == "open-or-create", "open failure should report the open-or-create operation");

		const auto clearFailure = Inbox::DescribeStartupClearResult({ .kind = Inbox::StartupClearResultKind::kFailedClear,
			.errorCode = 5 });
		Expect(clearFailure.failedOperation == "clear", "clear failure should report the clear operation");
	}

	void TestStartupInboxClearCreatesEmptyInboxWhenMissing()
	{
		const auto path = MakeTempInboxPath();
		std::filesystem::remove(path);

		const auto result = Inbox::ClearAtStartup(path);
		Expect(result.kind == Inbox::StartupClearResultKind::kEstablished, "missing inbox should be created successfully");
		Expect(std::filesystem::exists(path), "startup inbox clear should create the missing inbox file");
		Expect(std::filesystem::file_size(path) == 0, "created inbox should be empty after startup inbox clear");

		std::filesystem::remove(path);
	}

	void TestStartupInboxClearTruncatesExistingInbox()
	{
		const auto path = MakeTempInboxPath();
		WriteFile(path, "stale\ncommands\n");

		const auto result = Inbox::ClearAtStartup(path);
		Expect(result.kind == Inbox::StartupClearResultKind::kEstablished, "existing inbox should be cleared successfully");
		Expect(std::filesystem::file_size(path) == 0, "existing inbox should be empty after startup inbox clear");

		std::filesystem::remove(path);
	}
}

void RunStartupInboxClearTests()
{
	TestSuccessActionResetsStateAndAnnouncesClear();
	TestFailureActionsDisableInboxProcessing();
	TestStartupInboxClearCreatesEmptyInboxWhenMissing();
	TestStartupInboxClearTruncatesExistingInbox();
}
