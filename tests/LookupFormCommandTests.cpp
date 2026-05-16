#include "local_commands/lookup/LookupFormCommand.h"

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

	void TestLookupFormRequiresFormKey()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand("", {});
		Expect(result.size() == 1, "missing form key should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-form <plugin>|<local-formid>", "missing form key should return usage");
	}

	void TestLookupFormRejectsMalformedKey()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand("nope", {});
		Expect(result.size() == 1, "malformed form key should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-form <plugin>|<local-formid>", "malformed form key should return usage");
	}

	void TestLookupFormRejectsNonHexFormToken()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand("Skyrim.esm|nothex", {});
		Expect(result.size() == 1, "non-hex form token should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-form <plugin>|<local-formid>", "non-hex form token should return usage");
	}

	void TestLookupFormRejectsNonLocalFormID()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand("Skyrim.esm|01000000", {});
		Expect(result.size() == 1, "non-local form ID should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-form <plugin>|<local-formid>", "non-local form ID should return usage");
	}

	void TestLookupFormReportsMiss()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand("Skyrim.esm|0003DF19", std::nullopt);
		Expect(result.size() == 1, "miss should return one response line");
		Expect(result.front() == "lookup-form \"Skyrim.esm|0003DF19\" -> miss", "miss should be explicit");
	}

	void TestLookupFormHitIncludesIdentifyingMetadata()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand(
			"Skyrim.esm|0003DF19",
			LocalCommands::LookupFormHit{
				.formID = 0x0003DF19,
				.formType = "LVLI",
				.editorID = "LItemBanditArmorHeavy",
				.name = "Bandit Heavy Armor",
				.source = "Skyrim.esm",
			});
		Expect(result.size() == 1, "hit should return one response line");
		Expect(
			result.front() ==
				"lookup-form \"Skyrim.esm|0003DF19\" -> hit form=0x0003DF19 type=LVLI editorID=\"LItemBanditArmorHeavy\" name=\"Bandit Heavy Armor\" source=\"Skyrim.esm\"",
			"hit should include form, type, editor ID, name, and source");
	}

	void TestLookupFormAllowsPluginFilenameWithSpaces()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand(
			"Sentinel - Master Plugin.esp|0003DF19",
			LocalCommands::LookupFormHit{
				.formID = 0x0103DF19,
				.formType = "LVLI",
				.editorID = "LItemBanditArmorHeavy",
				.name = "Bandit Heavy Armor",
				.source = "Sentinel - Master Plugin.esp",
			});
		Expect(result.size() == 1, "space-containing plugin filename should still return one response line");
		Expect(
			result.front() ==
				"lookup-form \"Sentinel - Master Plugin.esp|0003DF19\" -> hit form=0x0103DF19 type=LVLI editorID=\"LItemBanditArmorHeavy\" name=\"Bandit Heavy Armor\" source=\"Sentinel - Master Plugin.esp\"",
			"space-containing plugin filename should be accepted without quoting");
	}

	void TestLookupFormOmitsMissingOptionalMetadata()
	{
		const auto result = LocalCommands::ExecuteLookupFormCommand(
			"Sentinel.esp|00000ABC",
			LocalCommands::LookupFormHit{
				.formID = 0x02000ABC,
				.formType = "ARMO",
				.source = "Sentinel.esp",
			});
		Expect(result.size() == 1, "hit should return one response line");
		Expect(
			result.front() ==
				"lookup-form \"Sentinel.esp|00000ABC\" -> hit form=0x02000ABC type=ARMO editorID=\"\" source=\"Sentinel.esp\"",
			"hit should render an empty editor ID explicitly and omit name when unavailable");
	}
}

void RunLookupFormCommandTests()
{
	TestLookupFormRequiresFormKey();
	TestLookupFormRejectsMalformedKey();
	TestLookupFormRejectsNonHexFormToken();
	TestLookupFormRejectsNonLocalFormID();
	TestLookupFormReportsMiss();
	TestLookupFormHitIncludesIdentifyingMetadata();
	TestLookupFormAllowsPluginFilenameWithSpaces();
	TestLookupFormOmitsMissingOptionalMetadata();
}
