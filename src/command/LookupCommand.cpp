#include "command/LookupCommand.h"

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
}

namespace Command
{
	std::vector<std::string> ExecuteLookupCommand(std::string_view arguments)
	{
		auto                   remaining = TrimAscii(arguments);
		const auto             editorID = ConsumeToken(remaining);
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

		std::string response = "lookup \"" + std::string(editorID) + "\" -> hit form=" + FormatFormID(form->GetFormID());
		response += " type=";
		response += DescribeFormType(form->GetFormType());

		const auto* name = form->GetName();
		if (name != nullptr && name[0] != '\0') {
			response += " name=\"";
			response += name;
			response += "\"";
		}

		responseLines.push_back(std::move(response));
		return responseLines;
	}
}
