#include "local_commands/lookup/LookupCommand.h"

#include "local_commands/support/FormFormatting.h"
#include "local_commands/support/LocalCommandParsing.h"
#include "pch.h"

namespace
{
	using LocalCommands::ConsumeToken;
	using LocalCommands::DescribeFormType;
	using LocalCommands::FormatFormID;
	using LocalCommands::TrimAscii;

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
