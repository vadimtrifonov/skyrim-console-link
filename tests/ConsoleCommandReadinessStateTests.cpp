#include "console_commands/ConsoleCommandReadinessState.h"

#include <stdexcept>
#include <string_view>

namespace
{
	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	void TestInitialFalseStateDoesNotAnnounceUnavailable()
	{
		ConsoleCommands::ConsoleCommandReadinessState state;
		Expect(
			state.Observe(false) == ConsoleCommands::ConsoleCommandReadinessTransition::kNone,
			"initial inactive readiness should not announce unavailable");
		Expect(!state.IsReady(), "initial false observation should keep the state not ready");
	}

	void TestReadyUnavailableTransitionsAreExplicit()
	{
		ConsoleCommands::ConsoleCommandReadinessState state;
		Expect(
			state.Observe(true) == ConsoleCommands::ConsoleCommandReadinessTransition::kBecameReady,
			"first ready observation should announce readiness");
		Expect(state.IsReady(), "ready observation should make the processor ready");
		Expect(
			state.Observe(false) == ConsoleCommands::ConsoleCommandReadinessTransition::kBecameUnavailable,
			"ready to not-ready transition should announce unavailability");
		Expect(!state.IsReady(), "unavailable transition should clear readiness");
		Expect(
			state.Observe(true) == ConsoleCommands::ConsoleCommandReadinessTransition::kBecameReady,
			"not-ready to ready transition should announce readiness again");
	}
}

void RunConsoleCommandReadinessStateTests()
{
	TestInitialFalseStateDoesNotAnnounceUnavailable();
	TestReadyUnavailableTransitionsAreExplicit();
}
