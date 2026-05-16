#include "local_commands/lookup_prefix/LookupPrefixCommand.h"

#include <stdexcept>
#include <string_view>

namespace
{
	using LocalCommands::LookupPrefixMatch;

	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	std::vector<LookupPrefixMatch> MakePrefixMatches()
	{
		return {
			LookupPrefixMatch{ .editorID = "ArmorSteelBoots", .formID = 0x00013962, .formType = "ARMO", .name = "Steel Boots", .source = "Skyrim.esm" },
			LookupPrefixMatch{ .editorID = "ArmorIronBoots", .formID = 0x00012E4F, .formType = "ARMO", .name = "Iron Boots", .source = "Sentinel.esp" },
			LookupPrefixMatch{ .editorID = "ArmorIronCuirass", .formID = 0x00012E49, .formType = "ARMO", .name = "Iron Armor", .source = "Sentinel - Master Plugin.esp" }
		};
	}

	void TestLookupPrefixRequiresPrefix()
	{
		const auto result = LocalCommands::ExecuteLookupPrefixCommand("", {});
		Expect(result.size() == 1, "missing prefix should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-prefix <prefix> [limit]", "missing prefix should return usage");
	}

	void TestLookupPrefixRejectsZeroLimit()
	{
		const auto result = LocalCommands::ExecuteLookupPrefixCommand("Armor 0", {});
		Expect(result.size() == 1, "zero limit should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-prefix <prefix> [limit]", "zero limit should return usage");
	}

	void TestLookupPrefixRejectsNonNumericLimit()
	{
		const auto result = LocalCommands::ExecuteLookupPrefixCommand("Armor nope", {});
		Expect(result.size() == 1, "non-numeric limit should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-prefix <prefix> [limit]", "non-numeric limit should return usage");
	}

	void TestLookupPrefixUsesDefaultLimitAndOrdersMatches()
	{
		const auto matches = MakePrefixMatches();
		const auto result = LocalCommands::ExecuteLookupPrefixCommand("Armor", matches);
		Expect(result.size() == 4, "default limit should emit summary plus all matches");
		Expect(result[0] == "lookup-prefix \"Armor\" -> 3 match(es)", "prefix lookup should emit match count summary");
		Expect(
			result[1] ==
				"lookup-prefix match editorID=\"ArmorIronBoots\" form=0x00012E4F type=ARMO name=\"Iron Boots\" source=\"Sentinel.esp\"",
			"prefix lookup should order the first emitted match by editor ID");
		Expect(
			result[2] ==
				"lookup-prefix match editorID=\"ArmorIronCuirass\" form=0x00012E49 type=ARMO name=\"Iron Armor\" source=\"Sentinel - Master Plugin.esp\"",
			"prefix lookup should order the second emitted match by editor ID");
		Expect(
			result[3] ==
				"lookup-prefix match editorID=\"ArmorSteelBoots\" form=0x00013962 type=ARMO name=\"Steel Boots\" source=\"Skyrim.esm\"",
			"prefix lookup should order the final emitted match by editor ID");
	}

	void TestLookupPrefixAppliesExplicitLimitAndReportsTruncation()
	{
		const auto matches = MakePrefixMatches();
		const auto result = LocalCommands::ExecuteLookupPrefixCommand("Armor 2", matches);
		Expect(result.size() == 4, "explicit limit should emit summary, limited matches, and truncation");
		Expect(result[0] == "lookup-prefix \"Armor\" -> 3 match(es)", "explicit limit should keep full match summary");
		Expect(
			result[1] ==
				"lookup-prefix match editorID=\"ArmorIronBoots\" form=0x00012E4F type=ARMO name=\"Iron Boots\" source=\"Sentinel.esp\"",
			"explicit limit should emit the first ordered match");
		Expect(
			result[2] ==
				"lookup-prefix match editorID=\"ArmorIronCuirass\" form=0x00012E49 type=ARMO name=\"Iron Armor\" source=\"Sentinel - Master Plugin.esp\"",
			"explicit limit should emit the second ordered match");
		Expect(
			result[3] == "lookup-prefix \"Armor\" -> truncated to 2 of 3 match(es)",
			"explicit limit should report truncation");
	}

	void TestLookupPrefixReportsMiss()
	{
		const auto result = LocalCommands::ExecuteLookupPrefixCommand("NoSuchPrefix", {});
		Expect(result.size() == 1, "miss should return one response line");
		Expect(result[0] == "lookup-prefix \"NoSuchPrefix\" -> miss", "miss should be explicit");
	}
}

void RunLookupPrefixCommandTests()
{
	TestLookupPrefixRequiresPrefix();
	TestLookupPrefixRejectsZeroLimit();
	TestLookupPrefixRejectsNonNumericLimit();
	TestLookupPrefixUsesDefaultLimitAndOrdersMatches();
	TestLookupPrefixAppliesExplicitLimitAndReportsTruncation();
	TestLookupPrefixReportsMiss();
}
