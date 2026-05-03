#include "local_commands/lookup_lvli/LookupLvli.h"
#include "local_commands/lookup_lvli/LookupLvliCommand.h"

#include <stdexcept>
#include <string_view>

namespace
{
	using LocalCommands::LookupLvliEntry;
	using LocalCommands::LookupLvliHit;
	using LocalCommands::LookupLvliResult;
	using LocalCommands::LookupLvliResultKind;
	using LocalCommands::LookupLvliWrongType;

	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	LookupLvliResult MakeHitResult(std::vector<LookupLvliEntry> entries)
	{
		LookupLvliResult result{};
		result.kind = LookupLvliResultKind::kHit;
		result.hit = LookupLvliHit{
			.formID = 0x01000ABC,
			.source = "Sentinel.esp",
			.entries = std::move(entries),
		};
		return result;
	}

	void TestLookupLvliRequiresEditorID()
	{
		const auto result = LocalCommands::ExecuteLookupLvliCommand("", {});
		Expect(result.size() == 1, "missing editor ID should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-lvli <EditorID> [limit]", "missing editor ID should return usage");
	}

	void TestLookupLvliRejectsZeroLimit()
	{
		const auto result = LocalCommands::ExecuteLookupLvliCommand("BanditArmorList 0", {});
		Expect(result.size() == 1, "zero limit should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-lvli <EditorID> [limit]", "zero limit should return usage");
	}

	void TestLookupLvliRejectsNonNumericLimit()
	{
		const auto result = LocalCommands::ExecuteLookupLvliCommand("BanditArmorList nope", {});
		Expect(result.size() == 1, "non-numeric limit should return one response line");
		Expect(result.front() == "usage: cdbg.lookup-lvli <EditorID> [limit]", "non-numeric limit should return usage");
	}

	void TestLookupLvliReportsMiss()
	{
		const auto result = LocalCommands::ExecuteLookupLvliCommand("MissingList", {});
		Expect(result.size() == 1, "miss should return one response line");
		Expect(result.front() == "lookup-lvli \"MissingList\" -> miss", "miss should be explicit");
	}

	void TestLookupLvliReportsWrongType()
	{
		LookupLvliResult result{};
		result.kind = LookupLvliResultKind::kWrongType;
		result.wrongType = LookupLvliWrongType{
			.formID = 0x00012E49,
			.formType = "ARMO",
			.source = "Skyrim.esm",
		};

		const auto response = LocalCommands::ExecuteLookupLvliCommand("TH_IronCuirass", result);
		Expect(response.size() == 1, "wrong type should return one response line");
		Expect(
			response.front() ==
				"lookup-lvli \"TH_IronCuirass\" -> wrong-type form=0x00012E49 type=ARMO source=\"Skyrim.esm\"",
			"wrong type should include form, type, and source");
	}

	void TestLookupLvliUsesDefaultLimitAndPreservesEntryOrder()
	{
		std::vector<LookupLvliEntry> entries;
		entries.reserve(12);
		for (std::uint32_t index = 0; index < 12; ++index) {
			entries.push_back(LookupLvliEntry{
				.level = static_cast<std::uint16_t>(index + 1),
				.count = static_cast<std::uint16_t>((index % 3) + 1),
				.formID = 0x02000010 + index,
				.formType = index == 1 ? "LVLI" : "ARMO",
				.editorID = index == 1 ? "NestedArmorList" : "ArmorEntry" + std::to_string(index + 1),
				.name = index == 1 ? "Nested Armor List" : "Armor Name " + std::to_string(index + 1),
				.source = index % 2 == 0 ? "Skyrim.esm" : "Sentinel.esp",
			});
		}

		const auto result = LocalCommands::ExecuteLookupLvliCommand("BanditArmorList", MakeHitResult(std::move(entries)));
		Expect(result.size() == 12, "default limit should emit summary, ten entries, and truncation");
		Expect(
			result[0] == "lookup-lvli \"BanditArmorList\" -> hit form=0x01000ABC entries=12 source=\"Sentinel.esp\"",
			"summary should include form, total entry count, and source");
		Expect(
			result[1] ==
				"lookup-lvli entry level=1 count=1 form=0x02000010 type=ARMO editorID=\"ArmorEntry1\" name=\"Armor Name 1\" source=\"Skyrim.esm\"",
			"default limit should emit the first direct entry in order");
		Expect(
			result[2] ==
				"lookup-lvli entry level=2 count=2 form=0x02000011 type=LVLI editorID=\"NestedArmorList\" name=\"Nested Armor List\" source=\"Sentinel.esp\"",
			"nested LVLI references should appear as one direct entry line");
		Expect(
			result[10] ==
				"lookup-lvli entry level=10 count=1 form=0x02000019 type=ARMO editorID=\"ArmorEntry10\" name=\"Armor Name 10\" source=\"Sentinel.esp\"",
			"default limit should preserve entry order up to the built-in limit");
		Expect(
			result[11] == "lookup-lvli \"BanditArmorList\" -> truncated to 10 of 12 entries",
			"default limit should report truncation");
	}

	void TestLookupLvliAppliesExplicitLimit()
	{
		const auto response = LocalCommands::ExecuteLookupLvliCommand(
			"BanditArmorList 2",
			MakeHitResult({
				LookupLvliEntry{
					.level = 4,
					.count = 1,
					.formID = 0x02000020,
					.formType = "ARMO",
					.editorID = "ArmorIronCuirass",
					.name = "Iron Armor",
					.source = "Skyrim.esm",
				},
				LookupLvliEntry{
					.level = 6,
					.count = 2,
					.formID = 0x02000021,
					.formType = "LVLI",
					.editorID = "NestedBanditArmorList",
					.name = "Nested Bandit Armor",
					.source = "Sentinel.esp",
				},
				LookupLvliEntry{
					.level = 8,
					.count = 1,
					.formID = 0x02000022,
					.formType = "WEAP",
					.editorID = "WeaponSteelSword",
					.name = "Steel Sword",
					.source = "Skyrim.esm",
				},
			}));

		Expect(response.size() == 4, "explicit limit should emit summary, limited entries, and truncation");
		Expect(
			response[0] == "lookup-lvli \"BanditArmorList\" -> hit form=0x01000ABC entries=3 source=\"Sentinel.esp\"",
			"explicit limit should keep the full summary count");
		Expect(
			response[1] ==
				"lookup-lvli entry level=4 count=1 form=0x02000020 type=ARMO editorID=\"ArmorIronCuirass\" name=\"Iron Armor\" source=\"Skyrim.esm\"",
			"explicit limit should emit the first entry");
		Expect(
			response[2] ==
				"lookup-lvli entry level=6 count=2 form=0x02000021 type=LVLI editorID=\"NestedBanditArmorList\" name=\"Nested Bandit Armor\" source=\"Sentinel.esp\"",
			"explicit limit should emit the second entry");
		Expect(
			response[3] == "lookup-lvli \"BanditArmorList\" -> truncated to 2 of 3 entries",
			"explicit limit should report truncation");
	}

	void TestLookupLvliOmitsMissingEditorIDAndName()
	{
		const auto response = LocalCommands::ExecuteLookupLvliCommand(
			"BanditArmorList",
			MakeHitResult({
				LookupLvliEntry{
					.level = 12,
					.count = 3,
					.formID = 0x02000030,
					.formType = "ARMO",
					.source = "Skyrim.esm",
				},
			}));

		Expect(response.size() == 2, "single entry hit should emit summary plus one entry");
		Expect(
			response[1] == "lookup-lvli entry level=12 count=3 form=0x02000030 type=ARMO source=\"Skyrim.esm\"",
			"entry lines should omit editor ID and name when unavailable");
	}
}

void RunLookupLvliCommandTests()
{
	TestLookupLvliRequiresEditorID();
	TestLookupLvliRejectsZeroLimit();
	TestLookupLvliRejectsNonNumericLimit();
	TestLookupLvliReportsMiss();
	TestLookupLvliReportsWrongType();
	TestLookupLvliUsesDefaultLimitAndPreservesEntryOrder();
	TestLookupLvliAppliesExplicitLimit();
	TestLookupLvliOmitsMissingEditorIDAndName();
}
