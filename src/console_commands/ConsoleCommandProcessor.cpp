#include "console_commands/ConsoleCommandProcessor.h"

#include "console/ConsoleMenuState.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include "RE/C/Console.h"
#include "RE/U/UI.h"

namespace
{
	struct State
	{
		bool ready = false;
		bool initialized = false;
	};

	State& GetState()
	{
		static State state;
		return state;
	}

	void UpdateReadiness(const bool ready) noexcept
	{
		auto& state = GetState();
		if (state.ready == ready) {
			return;
		}

		state.ready = ready;
		Logging::LogLocalActivityLine(ready ? "console commands are ready" : "console commands are not ready");
	}

	void OnMenuStateChange(const bool isOpen) noexcept
	{
		if (!GetState().initialized) {
			return;
		}

		UpdateReadiness(isOpen);
	}
}

namespace ConsoleCommands
{
	void Initialize() noexcept
	{
		auto& state = GetState();
		if (state.initialized) {
			return;
		}

		state.initialized = true;
		Console::MenuState::AddStateChangeHandler(&::OnMenuStateChange);
		UpdateReadiness(Console::MenuState::IsConsoleMenuOpen());
	}

	ProcessResult Process(std::string_view line) noexcept
	{
		try {
			if (line.empty()) {
				Logging::LogLocalActivityLine("empty console command");
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

			if (!ui->GetMenu<RE::Console>()) {
				logs::warn(
					"Console command submission is rejecting commands because the active Console menu object is unavailable");
				Logging::LogLocalActivityLine("console commands are not ready");
				return ProcessResult::kRejected;
			}

			if (line.find('\0') != std::string_view::npos) {
				logs::warn("Console command submission rejected a command containing an embedded NUL byte");
				Logging::LogLocalActivityLine("console command contains an embedded NUL byte");
				return ProcessResult::kRejected;
			}

			const std::string command(line);
			RE::Console::ExecuteCommand(command.c_str());

			UpdateReadiness(true);
			Logging::LogConsoleSubmitLine(line);
			return ProcessResult::kHandled;
		} catch (const std::exception& exception) {
			logs::warn("Console command processor failed: {}", exception.what());
		} catch (...) {
			logs::warn("Console command processor failed: unknown exception");
		}

		return ProcessResult::kRejected;
	}
}
