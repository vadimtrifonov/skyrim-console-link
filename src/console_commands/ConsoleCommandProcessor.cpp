#include "console_commands/ConsoleCommandProcessor.h"

#include "console_commands/ConsoleCommandReadinessState.h"
#include "logging/ConsoleActivityLog.h"
#include "observation/ConsoleInputObservation.h"
#include "pch.h"

#include "RE/C/Console.h"
#include "RE/U/UI.h"

namespace
{
	void SwallowFailSoftBoundary() noexcept {}

	struct ReadinessSnapshot
	{
		bool ready = false;
		std::string_view rejectionMessage = "console commands are not ready";
	};

	struct ConsoleCommandState
	{
		ConsoleCommands::ConsoleCommandReadinessState readiness;
		bool terminalFailure = false;
		bool initialized = false;
	};

	ConsoleCommandState& GetState()
	{
		static ConsoleCommandState state;
		return state;
	}

	ReadinessSnapshot SnapshotReadiness() noexcept
	{
		auto& state = GetState();
		if (state.terminalFailure || !REL::Module::IsVR() || !Observation::Input::CanSubmitCommands()) {
			return { false, "console command submission is unavailable" };
		}

		auto* ui = RE::UI::GetSingleton();
		if (ui == nullptr) {
			return { false, "console commands are not ready" };
		}

		auto console = ui->GetMenu<RE::Console>();
		if (!console || !console->uiMovie) {
			return { false, "console commands are not ready" };
		}

		return { true, {} };
	}

	void UpdateReadinessTransition(const ReadinessSnapshot& snapshot) noexcept
	{
		auto& state = GetState();
		switch (state.readiness.Observe(snapshot.ready)) {
		case ConsoleCommands::ConsoleCommandReadinessTransition::kBecameReady:
			Logging::LogLocalActivityLine("console commands are ready");
			break;
		case ConsoleCommands::ConsoleCommandReadinessTransition::kBecameUnavailable:
			Logging::LogLocalActivityLine("console commands are unavailable");
			break;
		case ConsoleCommands::ConsoleCommandReadinessTransition::kNone:
		default:
			break;
		}
	}
}

namespace ConsoleCommands
{
	void Initialize() noexcept
	{
		GetState().initialized = true;
	}

	void PollReadiness() noexcept
	{
		if (!GetState().initialized) {
			return;
		}

		UpdateReadinessTransition(SnapshotReadiness());
	}

	ProcessResult Process(std::string_view line) noexcept
	{
		try {
			const auto snapshot = SnapshotReadiness();
			UpdateReadinessTransition(snapshot);
			if (!snapshot.ready) {
				Logging::LogLocalActivityLine(snapshot.rejectionMessage);
				return ProcessResult::kRejected;
			}

			const auto result = Observation::Input::SubmitCommand(line);
			switch (result) {
			case Observation::Input::SubmitCommandResult::kSubmitted:
				return ProcessResult::kHandled;
			case Observation::Input::SubmitCommandResult::kDeferred:
				Logging::LogLocalActivityLine("console commands are not ready");
				UpdateReadinessTransition({ false, "console commands are not ready" });
				return ProcessResult::kRejected;
			case Observation::Input::SubmitCommandResult::kTerminalFailure:
			default:
				GetState().terminalFailure = true;
				UpdateReadinessTransition({ false, "console command submission is unavailable" });
				Logging::LogLocalActivityLine("console command submission is unavailable");
				return ProcessResult::kRejected;
			}
		} catch (const std::exception& exception) {
			try {
				logs::warn("Console command processor failed: {}", exception.what());
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		} catch (...) {
			try {
				logs::warn("Console command processor failed: unknown exception");
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		}

		GetState().terminalFailure = true;
		UpdateReadinessTransition({ false, "console command submission is unavailable" });
		Logging::LogLocalActivityLine("console command submission is unavailable");
		return ProcessResult::kRejected;
	}
}
