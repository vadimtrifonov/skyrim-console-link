#include "inbox/InboxFileReader.h"
#include "inbox/InboxState.h"
#include "inbox/StartupInboxClear.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

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

	Inbox::FileObservation ReadObservation(const std::filesystem::path& path)
	{
		const Inbox::FileReader reader(path);
		auto opened = reader.Open();
		if (const auto* error = std::get_if<Inbox::FileError>(&opened)) {
			throw std::system_error(static_cast<int>(error->code), std::system_category(), std::string(error->operation) + " inbox '" + path.string() + "' for observation");
		}
		Expect(std::holds_alternative<Inbox::OpenFile>(opened), "inbox should open for observation");
		return std::get<Inbox::OpenFile>(opened).Observation();
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

		const auto inspectFailure = Inbox::DescribeStartupClearResult({ .kind = Inbox::StartupClearResultKind::kFailedInspect,
			.errorCode = 5 });
		Expect(!inspectFailure.shouldProceed && !inspectFailure.shouldResetState, "observation failure must not establish inbox readiness");
		Expect(inspectFailure.failedOperation == "observe", "observation failure should identify the failed metadata query");

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
		Expect(result.identity == ReadObservation(path).identity, "startup clear should return the created file's identity");

		std::filesystem::remove(path);
	}

	void TestStartupInboxClearTruncatesExistingInbox()
	{
		const auto path = MakeTempInboxPath();
		WriteFile(path, "stale\ncommands\n");
		const auto original = ReadObservation(path);

		const auto result = Inbox::ClearAtStartup(path);
		Expect(result.kind == Inbox::StartupClearResultKind::kEstablished, "existing inbox should be cleared successfully");
		Expect(std::filesystem::file_size(path) == 0, "existing inbox should be empty after startup inbox clear");
		Expect(result.identity == original.identity && result.identity == ReadObservation(path).identity, "startup clear should preserve and return the cleared file's identity");

		std::filesystem::remove(path);
	}

	void TestReplacementBeforeFirstPollAcceptsFinalLine()
	{
		const auto path = MakeTempInboxPath();
		const auto replacement = MakeTempInboxPath();
		WriteFile(path, "stale\ncommands\n");
		const auto cleared = Inbox::ClearAtStartup(path);
		Expect(cleared.kind == Inbox::StartupClearResultKind::kEstablished, "startup clear should succeed before replacement");

		Inbox::State state;
		state.ResetForNewSession(cleared.identity);

		const std::string command = "cdbg.lookup-form Skyrim.esm|00000007";
		WriteFile(replacement, command);
		if (!::MoveFileExW(replacement.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
			const auto error = ::GetLastError();
			throw std::system_error(static_cast<int>(error), std::system_category(), "replace inbox '" + path.string() + "' with '" + replacement.string() + "'");
		}

		const Inbox::FileReader reader(path);
		auto opened = reader.Open();
		if (const auto* error = std::get_if<Inbox::FileError>(&opened)) {
			throw std::system_error(static_cast<int>(error->code), std::system_category(), std::string(error->operation) + " replaced inbox '" + path.string() + "'");
		}
		Expect(std::holds_alternative<Inbox::OpenFile>(opened), "replaced inbox should open");
		const auto& file = std::get<Inbox::OpenFile>(opened);
		const auto observed = state.ObserveFile(file.Observation());
		auto read = file.ReadAt(0, file.Observation().size);
		if (const auto* error = std::get_if<Inbox::FileError>(&read)) {
			throw std::system_error(static_cast<int>(error->code), std::system_category(), std::string(error->operation) + " replacement contents '" + path.string() + "'");
		}
		std::filesystem::remove(path);

		Expect(observed.readMode == Inbox::ReadMode::kRewrite, "replacement before first poll should use whole-file ingestion");
		const auto accepted = state.AcceptRewriteContents(file.Observation(), std::get<std::string>(read));
		Expect(accepted.extractedCount == 1, "replacement before first poll should accept a final line without a newline");
		const auto line = state.FrontLine();
		Expect(line.has_value() && line->text == command, "replacement command should be queued intact");
		Expect(state.PopFrontLine(line->sequence), "replacement command should be consumable");
		Expect(state.ObserveFile(file.Observation()).readMode == Inbox::ReadMode::kNone, "unchanged replacement should not be replayed");
		Expect(!state.HasPendingLines(), "replacement should be consumed exactly once");
	}
}

void RunStartupInboxClearTests()
{
	TestSuccessActionResetsStateAndAnnouncesClear();
	TestFailureActionsDisableInboxProcessing();
	TestStartupInboxClearCreatesEmptyInboxWhenMissing();
	TestStartupInboxClearTruncatesExistingInbox();
	TestReplacementBeforeFirstPollAcceptsFinalLine();
}
