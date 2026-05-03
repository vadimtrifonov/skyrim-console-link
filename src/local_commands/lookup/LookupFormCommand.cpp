#include "local_commands/lookup/LookupFormCommand.h"

#include "local_commands/support/FormFormatting.h"
#include "local_commands/support/FormKey.h"
#include "local_commands/support/LocalCommandParsing.h"
#include "pch.h"

#include <RE/T/TESDataHandler.h>

#include <stdexcept>

namespace
{
	using LocalCommands::DescribeFormType;
	using LocalCommands::FormatFormID;
	using LocalCommands::TrimAscii;

	[[nodiscard]] std::optional<std::string> ParseLookupFormArgument(std::string_view arguments) noexcept
	{
		const auto formKey = TrimAscii(arguments);
		if (formKey.empty() || !LocalCommands::ParseFormKey(formKey).has_value()) {
			return std::nullopt;
		}

		return std::string(formKey);
	}

	std::string BuildLookupFormHitLine(
		std::string_view formKeyText,
		const LocalCommands::LookupFormHit& hit)
	{
		std::string response =
			"lookup-form \"" + std::string(formKeyText) + "\" -> hit form=" + FormatFormID(hit.formID);
		response += " type=";
		response += hit.formType;

		response += " editorID=\"";
		response += hit.editorID;
		response += "\"";

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
	std::vector<std::string> ExecuteLookupFormCommand(std::string_view arguments)
	{
		const auto formKey = ParseLookupFormArgument(arguments);
		if (!formKey.has_value()) {
			return { "usage: cdbg.lookup-form <plugin>|<local-formid>" };
		}

		const auto parsedKey = ParseFormKey(*formKey);

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (dataHandler == nullptr) {
			throw std::runtime_error("cdbg.lookup-form could not access TESDataHandler");
		}

		const auto* form = LookupFormByFormKey(*parsedKey, *dataHandler);
		if (form == nullptr) {
			return ExecuteLookupFormCommand(arguments, std::nullopt);
		}

		LookupFormHit hit{};
		hit.formID = form->GetFormID();
		hit.formType = std::string(DescribeFormType(form->GetFormType()));

		if (const auto* editorID = form->GetFormEditorID(); editorID != nullptr && editorID[0] != '\0') {
			hit.editorID = editorID;
		}

		if (const auto* name = form->GetName(); name != nullptr && name[0] != '\0') {
			hit.name = name;
		}

		if (const auto* file = form->GetFile(); file != nullptr) {
			hit.source = std::string(file->GetFilename());
		}

		return ExecuteLookupFormCommand(arguments, std::move(hit));
	}

	std::vector<std::string> ExecuteLookupFormCommand(
		std::string_view arguments,
		std::optional<LookupFormHit> hit)
	{
		const auto formKey = ParseLookupFormArgument(arguments);
		if (!formKey.has_value()) {
			return { "usage: cdbg.lookup-form <plugin>|<local-formid>" };
		}

		if (!hit.has_value()) {
			return { "lookup-form \"" + *formKey + "\" -> miss" };
		}

		return { BuildLookupFormHitLine(*formKey, *hit) };
	}
}
