#include "local_commands/LookupCommand.h"

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
}

void RunLookupCommandTests()
{
	TestLookupRequiresEditorID();
	TestLookupRejectsExtraArguments();
}
