#include "local_commands/lookup/LookupCommand.h"

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

	void TestLookupRequiresEditorID()
	{
		const auto result = LocalCommands::ExecuteLookupCommand("");
		Expect(result.size() == 1, "missing editor ID should return one response line");
		Expect(result.front() == "usage: cdbg.lookup <EditorID>", "missing editor ID should return usage");
	}

	void TestLookupRejectsExtraArguments()
	{
		const auto result = LocalCommands::ExecuteLookupCommand("TH_IronCuirass extra");
		Expect(result.size() == 1, "extra lookup arguments should return one response line");
		Expect(result.front() == "usage: cdbg.lookup <EditorID>", "extra lookup arguments should return usage");
	}

	void TestLookupHitIncludesSource()
	{
		const auto result = LocalCommands::ExecuteLookupCommand(
			"TH_IronCuirass",
			LocalCommands::LookupHit{
				.formID = 0x00012E49,
				.formType = "ARMO",
				.name = "Iron Armor",
				.source = "Skyrim.esm",
			});
		Expect(result.size() == 1, "lookup hit should return one response line");
		Expect(
			result.front() ==
				"lookup \"TH_IronCuirass\" -> hit form=0x00012E49 type=ARMO name=\"Iron Armor\" source=\"Skyrim.esm\"",
			"lookup hit should include the source plugin");
	}
}

void RunLookupCommandTests()
{
	TestLookupRequiresEditorID();
	TestLookupRejectsExtraArguments();
	TestLookupHitIncludesSource();
}
