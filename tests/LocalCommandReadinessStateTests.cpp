#include "local_commands/LocalCommandReadinessState.h"

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

	void TestDataLoadedTransitionsToReadyOnce()
	{
		LocalCommands::LocalCommandReadinessState state;

		Expect(!state.IsReady(), "local commands should start not ready");
		Expect(
			state.ObserveDataLoaded() == LocalCommands::LocalCommandReadinessTransition::kBecameReady,
			"first data-loaded observation should announce readiness");
		Expect(state.IsReady(), "local commands should become ready after data load");
		Expect(
			state.ObserveDataLoaded() == LocalCommands::LocalCommandReadinessTransition::kNone,
			"later data-loaded observations should not re-announce readiness");
	}
}

void RunLocalCommandReadinessStateTests()
{
	TestDataLoadedTransitionsToReadyOnce();
}
