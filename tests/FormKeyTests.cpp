#include "local_commands/support/FormKey.h"

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

	void TestParseFormKeyAcceptsPluginAndHexFormID()
	{
		const auto parsed = LocalCommands::ParseFormKey("Skyrim.esm|0003DF19");
		Expect(parsed.has_value(), "valid form key should parse");
		Expect(parsed->plugin == "Skyrim.esm", "parsed form key should preserve plugin filename");
		Expect(parsed->formID == 0x0003DF19, "parsed form key should parse hex form ID");
	}

	void TestParseFormKeyAllowsPluginFilenameWithSpaces()
	{
		const auto parsed = LocalCommands::ParseFormKey("Sentinel - Master Plugin.esp|00000ABC");
		Expect(parsed.has_value(), "plugin filename with spaces should parse");
		Expect(parsed->plugin == "Sentinel - Master Plugin.esp", "plugin filename with spaces should be preserved");
		Expect(parsed->formID == 0x00000ABC, "form ID should parse for plugin filename with spaces");
	}

	void TestParseFormKeyRejectsMissingSeparator()
	{
		Expect(!LocalCommands::ParseFormKey("Skyrim.esm 0003DF19").has_value(), "missing separator should not parse");
	}

	void TestParseFormKeyRejectsEmptyPlugin()
	{
		Expect(!LocalCommands::ParseFormKey("|0003DF19").has_value(), "empty plugin should not parse");
	}

	void TestParseFormKeyRejectsEmptyFormID()
	{
		Expect(!LocalCommands::ParseFormKey("Skyrim.esm|").has_value(), "empty form ID should not parse");
	}

	void TestParseFormKeyRejectsMultipleSeparators()
	{
		Expect(!LocalCommands::ParseFormKey("Skyrim.esm|0003DF19|extra").has_value(), "multiple separators should not parse");
	}

	void TestParseFormKeyRejectsNonHexFormID()
	{
		Expect(!LocalCommands::ParseFormKey("Skyrim.esm|nothex").has_value(), "non-hex form ID should not parse");
	}

	void TestParseFormKeyRejectsNonLocalFormID()
	{
		Expect(!LocalCommands::ParseFormKey("Skyrim.esm|01000000").has_value(), "non-local form ID should not parse");
	}
}

void RunFormKeyTests()
{
	TestParseFormKeyAcceptsPluginAndHexFormID();
	TestParseFormKeyAllowsPluginFilenameWithSpaces();
	TestParseFormKeyRejectsMissingSeparator();
	TestParseFormKeyRejectsEmptyPlugin();
	TestParseFormKeyRejectsEmptyFormID();
	TestParseFormKeyRejectsMultipleSeparators();
	TestParseFormKeyRejectsNonHexFormID();
	TestParseFormKeyRejectsNonLocalFormID();
}
