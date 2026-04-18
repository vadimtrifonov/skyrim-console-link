#include "local_commands/LookupLvliCommand.h"

#include "pch.h"

#include <RE/RTTI.h>
#include <RE/T/TESLevItem.h>

#include <charconv>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
	constexpr std::size_t kDefaultLookupLvliLimit = 10;

	struct LookupLvliArguments
	{
		std::string editorID;
		std::size_t limit = kDefaultLookupLvliLimit;
	};

	std::string_view TrimAscii(std::string_view value) noexcept
	{
		const auto isSpace = [](const unsigned char ch) noexcept {
			return std::isspace(ch) != 0;
		};

		while (!value.empty() && isSpace(static_cast<unsigned char>(value.front()))) {
			value.remove_prefix(1);
		}

		while (!value.empty() && isSpace(static_cast<unsigned char>(value.back()))) {
			value.remove_suffix(1);
		}

		return value;
	}

	std::string_view ConsumeToken(std::string_view& value) noexcept
	{
		value = TrimAscii(value);
		const auto tokenEnd = value.find_first_of(" \t");
		if (tokenEnd == std::string_view::npos) {
			const auto token = value;
			value = {};
			return token;
		}

		const auto token = value.substr(0, tokenEnd);
		value.remove_prefix(tokenEnd);
		value = TrimAscii(value);
		return token;
	}

	std::string FormatFormID(const std::uint32_t formID)
	{
		std::ostringstream stream;
		stream << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << formID;
		return stream.str();
	}

	std::string_view DescribeFormType(const RE::FormType formType) noexcept
	{
		switch (formType) {
		case RE::FormType::LeveledItem:
			return "LVLI";
		case RE::FormType::Weapon:
			return "WEAP";
		case RE::FormType::Armor:
			return "ARMO";
		case RE::FormType::NPC:
			return "NPC_";
		case RE::FormType::Outfit:
			return "OTFT";
		case RE::FormType::Keyword:
			return "KYWD";
		case RE::FormType::ConstructibleObject:
			return "COBJ";
		default:
			return "FORM";
		}
	}

	[[nodiscard]] std::optional<std::size_t> ParsePositiveLimit(std::string_view token) noexcept
	{
		if (token.empty()) {
			return std::nullopt;
		}

		std::size_t value = 0;
		const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
		if (ec != std::errc() || ptr != token.data() + token.size() || value == 0) {
			return std::nullopt;
		}

		return value;
	}

	[[nodiscard]] std::optional<LookupLvliArguments> ParseLookupLvliArguments(std::string_view arguments) noexcept
	{
		auto remaining = TrimAscii(arguments);
		const auto editorID = ConsumeToken(remaining);
		const auto limitToken = ConsumeToken(remaining);

		if (editorID.empty() || !remaining.empty()) {
			return std::nullopt;
		}

		LookupLvliArguments parsed{};
		parsed.editorID = std::string(editorID);
		if (limitToken.empty()) {
			return parsed;
		}

		const auto limit = ParsePositiveLimit(limitToken);
		if (!limit.has_value()) {
			return std::nullopt;
		}

		parsed.limit = *limit;
		return parsed;
	}

	std::string BuildLookupLvliWrongTypeLine(
		std::string_view editorID,
		const LocalCommands::LookupLvliWrongType& wrongType)
	{
		std::string response =
			"lookup-lvli \"" + std::string(editorID) + "\" -> wrong-type form=" + FormatFormID(wrongType.formID);
		response += " type=";
		response += wrongType.formType;
		response += " source=\"";
		response += wrongType.source;
		response += "\"";
		return response;
	}

	std::string BuildLookupLvliHitLine(
		std::string_view editorID,
		const LocalCommands::LookupLvliHit& hit)
	{
		std::string response =
			"lookup-lvli \"" + std::string(editorID) + "\" -> hit form=" + FormatFormID(hit.formID);
		response += " entries=";
		response += std::to_string(hit.entries.size());
		response += " source=\"";
		response += hit.source;
		response += "\"";
		return response;
	}

	std::string FormatLookupLvliEntryLine(const LocalCommands::LookupLvliEntry& entry)
	{
		std::string response = "lookup-lvli entry level=";
		response += std::to_string(entry.level);
		response += " count=";
		response += std::to_string(entry.count);
		response += " form=";
		response += FormatFormID(entry.formID);
		response += " type=";
		response += entry.formType;

		if (!entry.editorID.empty()) {
			response += " editor=\"";
			response += entry.editorID;
			response += "\"";
		}

		if (!entry.name.empty()) {
			response += " name=\"";
			response += entry.name;
			response += "\"";
		}

		response += " source=\"";
		response += entry.source;
		response += "\"";
		return response;
	}

	LocalCommands::LookupLvliResult ResolveLookupLvli(std::string_view editorID)
	{
		LocalCommands::LookupLvliResult result{};
		const auto* form = RE::TESForm::LookupByEditorID(editorID);
		if (form == nullptr) {
			return result;
		}

		if (form->GetFormType() != RE::FormType::LeveledItem) {
			result.kind = LocalCommands::LookupLvliResultKind::kWrongType;
			result.wrongType.formID = form->GetFormID();
			result.wrongType.formType = std::string(DescribeFormType(form->GetFormType()));
			if (const auto* file = form->GetFile(); file != nullptr) {
				result.wrongType.source = std::string(file->GetFilename());
			}
			return result;
		}

		const auto* leveledItem = skyrim_cast<const RE::TESLevItem*>(form);
		if (leveledItem == nullptr) {
			throw std::runtime_error("cdbg.lookup-lvli resolved LVLI form that could not be cast to TESLevItem");
		}

		result.kind = LocalCommands::LookupLvliResultKind::kHit;
		result.hit.formID = form->GetFormID();
		if (const auto* file = form->GetFile(); file != nullptr) {
			result.hit.source = std::string(file->GetFilename());
		}

		result.hit.entries.reserve(leveledItem->entries.size());
		for (const auto& object : leveledItem->entries) {
			LocalCommands::LookupLvliEntry entry{};
			entry.level = object.level;
			entry.count = object.count;

			if (const auto* entryForm = object.form; entryForm != nullptr) {
				entry.formID = entryForm->GetFormID();
				entry.formType = std::string(DescribeFormType(entryForm->GetFormType()));
				if (const auto* entryEditorID = entryForm->GetFormEditorID(); entryEditorID != nullptr && entryEditorID[0] != '\0') {
					entry.editorID = entryEditorID;
				}
				if (const auto* entryName = entryForm->GetName(); entryName != nullptr && entryName[0] != '\0') {
					entry.name = entryName;
				}
				if (const auto* entryFile = entryForm->GetFile(); entryFile != nullptr) {
					entry.source = std::string(entryFile->GetFilename());
				}
			} else {
				entry.formType = "FORM";
			}

			result.hit.entries.push_back(std::move(entry));
		}

		return result;
	}
}

namespace LocalCommands
{
	std::vector<std::string> ExecuteLookupLvliCommand(std::string_view arguments)
	{
		const auto parsed = ParseLookupLvliArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-lvli <EditorID> [limit]" };
		}

		return ExecuteLookupLvliCommand(arguments, ResolveLookupLvli(parsed->editorID));
	}

	std::vector<std::string> ExecuteLookupLvliCommand(
		std::string_view arguments,
		const LookupLvliResult& result)
	{
		const auto parsed = ParseLookupLvliArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-lvli <EditorID> [limit]" };
		}

		std::vector<std::string> responseLines;
		switch (result.kind) {
		case LookupLvliResultKind::kMiss:
			responseLines.push_back("lookup-lvli \"" + parsed->editorID + "\" -> miss");
			return responseLines;
		case LookupLvliResultKind::kWrongType:
			responseLines.push_back(BuildLookupLvliWrongTypeLine(parsed->editorID, result.wrongType));
			return responseLines;
		case LookupLvliResultKind::kHit:
			break;
		default:
			throw std::runtime_error("unknown lookup-lvli result kind");
		}

		const auto emittedCount = std::min(parsed->limit, result.hit.entries.size());
		responseLines.reserve(emittedCount + 2);
		responseLines.push_back(BuildLookupLvliHitLine(parsed->editorID, result.hit));

		for (std::size_t index = 0; index < emittedCount; ++index) {
			responseLines.push_back(FormatLookupLvliEntryLine(result.hit.entries[index]));
		}

		if (result.hit.entries.size() > emittedCount) {
			responseLines.push_back(
				"lookup-lvli \"" + parsed->editorID + "\" -> truncated to " + std::to_string(emittedCount) +
				" of " + std::to_string(result.hit.entries.size()) + " entries");
		}

		return responseLines;
	}
}
