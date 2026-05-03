#include "console_commands/ConsoleCommandProcessor.h"

#include "console/ConsoleMenuState.h"
#include "console/ExecuteCommandSeam.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include "RE/C/Console.h"
#include "RE/F/FxDelegateArgs.h"
#include "RE/U/UI.h"

namespace
{
	enum class ReadinessState
	{
		kReady,
		kNotReady,
		kUnavailable
	};

	struct State
	{
		ReadinessState readiness = ReadinessState::kNotReady;
		bool initialized = false;
	};

	State& GetState()
	{
		static State state;
		return state;
	}

	void UpdateReadiness(const ReadinessState readiness) noexcept
	{
		auto& state = GetState();
		if (state.readiness == readiness) {
			return;
		}

		state.readiness = readiness;
		switch (readiness) {
		case ReadinessState::kReady:
			Logging::LogLocalActivityLine("console commands are ready");
			break;
		case ReadinessState::kNotReady:
			Logging::LogLocalActivityLine("console commands are not ready");
			break;
		case ReadinessState::kUnavailable:
		default:
			Logging::LogLocalActivityLine("console commands are unavailable");
			break;
		}
	}

	void OnMenuStateChange(const bool isOpen) noexcept
	{
		auto& state = GetState();
		if (!state.initialized || state.readiness == ReadinessState::kUnavailable) {
			return;
		}

		UpdateReadiness(isOpen ? ReadinessState::kReady : ReadinessState::kNotReady);
	}
}

namespace ConsoleCommands
{
	void Initialize(const Settings::Values&) noexcept
	{
		auto& state = GetState();
		if (state.initialized) {
			return;
		}

		state.initialized = true;
		if (!REL::Module::IsVR()) {
			logs::warn("Skipping console command submission seam validation: the ExecuteCommand seam is VR-only");
			state.readiness = ReadinessState::kUnavailable;
			return;
		}

		if (!Console::ExecuteCommandSeam::Validate()) {
			state.readiness = ReadinessState::kUnavailable;
			return;
		}

		Console::MenuState::SetStateChangeHandler(&::OnMenuStateChange);
		UpdateReadiness(
			Console::MenuState::IsConsoleMenuOpen() ?
				ReadinessState::kReady :
				ReadinessState::kNotReady);
	}

	ProcessResult Process(std::string_view line) noexcept
	{
		try {
			if (line.empty()) {
				Logging::LogLocalActivityLine("empty console command");
				return ProcessResult::kRejected;
			}

			if (GetState().readiness == ReadinessState::kUnavailable) {
				Logging::LogLocalActivityLine("console commands are unavailable");
				return ProcessResult::kRejected;
			}

			auto* ui = RE::UI::GetSingleton();
			if (ui == nullptr) {
				logs::warn("Console command submission is rejecting commands because RE::UI is not available");
				Logging::LogLocalActivityLine("console commands are not ready");
				return ProcessResult::kRejected;
			}

			if (!Console::MenuState::IsConsoleMenuOpen()) {
				logs::warn("Console command submission is rejecting commands because the console menu is not open");
				Logging::LogLocalActivityLine("console commands are not ready");
				return ProcessResult::kRejected;
			}

			auto console = ui->GetMenu<RE::Console>();
			if (!console) {
				logs::warn("Console command submission is rejecting commands because the console menu is not open");
				Logging::LogLocalActivityLine("console commands are not ready");
				return ProcessResult::kRejected;
			}

			auto movie = console->uiMovie;
			if (!movie) {
				logs::warn("Console command submission is rejecting commands because the console movie is not available");
				Logging::LogLocalActivityLine("console commands are not ready");
				return ProcessResult::kRejected;
			}

			std::array<RE::GFxValue, 1> args{ RE::GFxValue(line) };
			RE::FxDelegateArgs delegateArgs(RE::GFxValue{}, console.get(), movie.get(), args.data(), static_cast<std::uint32_t>(args.size()));
			if (Console::ExecuteCommandSeam::Invoke(delegateArgs) != Console::ExecuteCommandSeam::InvokeResult::kInvoked) {
				logs::warn("Disabling console command submission after failure: ExecuteCommand seam invocation failed");
				UpdateReadiness(ReadinessState::kUnavailable);
				return ProcessResult::kRejected;
			}

			if (GetState().readiness != ReadinessState::kReady) {
				logs::warn("Console command submission succeeded without processor being ready; the readiness state was stale");
			}

			UpdateReadiness(ReadinessState::kReady);
			Logging::LogConsoleSubmitLine(line);
			return ProcessResult::kHandled;
		} catch (const std::exception& exception) {
			logs::warn("Console command processor failed: {}", exception.what());
		} catch (...) {
			logs::warn("Console command processor failed: unknown exception");
		}

		UpdateReadiness(ReadinessState::kUnavailable);
		return ProcessResult::kRejected;
	}
}
