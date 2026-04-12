#include "local_commands/LookupCommand.h"

#include "pch.h"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace
{
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

	std::string BuildLookupHitLine(
		std::string_view editorID,
		const LocalCommands::LookupHit& hit)
	{
		std::string response =
			"lookup \"" + std::string(editorID) + "\" -> hit form=" + FormatFormID(hit.formID);
		response += " type=";
		response += hit.formType;

		if (!hit.name.empty()) {
			response += " name=\"";
			response += hit.name;
			response += "\"";
		}

		response += " source=\"";
		response += hit.source;
		response += "\"";
		return response;
	}
}

namespace LocalCommands
{
	std::vector<std::string> ExecuteLookupCommand(std::string_view arguments)
	{
		auto remaining = TrimAscii(arguments);
		const auto editorID = ConsumeToken(remaining);
		std::vector<std::string> responseLines;

		if (editorID.empty() || !remaining.empty()) {
			responseLines.push_back("usage: cdbg.lookup <EditorID>");
			return responseLines;
		}

		const auto* form = RE::TESForm::LookupByEditorID(editorID);
		if (form == nullptr) {
			responseLines.push_back("lookup \"" + std::string(editorID) + "\" -> miss");
			return responseLines;
		}

		LookupHit hit{};
		hit.formID = form->GetFormID();
		hit.formType = std::string(DescribeFormType(form->GetFormType()));

		const auto* name = form->GetName();
		if (name != nullptr && name[0] != '\0') {
			hit.name = name;
		}

		if (const auto* file = form->GetFile(); file != nullptr) {
			hit.source = std::string(file->GetFilename());
		}

		return ExecuteLookupCommand(arguments, std::move(hit));
	}

	std::vector<std::string> ExecuteLookupCommand(
		std::string_view arguments,
		std::optional<LookupHit> hit)
	{
		auto remaining = TrimAscii(arguments);
		const auto editorID = ConsumeToken(remaining);
		std::vector<std::string> responseLines;

		if (editorID.empty() || !remaining.empty()) {
			responseLines.push_back("usage: cdbg.lookup <EditorID>");
			return responseLines;
		}

		if (!hit.has_value()) {
			responseLines.push_back("lookup \"" + std::string(editorID) + "\" -> miss");
			return responseLines;
		}

		responseLines.push_back(BuildLookupHitLine(editorID, *hit));
		return responseLines;
	}
}
