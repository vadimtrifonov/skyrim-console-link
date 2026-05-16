#include "local_commands/lookup_lvli/LookupLvliFormat.h"

#include "local_commands/support/FormFormatting.h"

#include <algorithm>
#include <stdexcept>

namespace
{
	using LocalCommands::FormatFormID;

	std::string BuildLookupLvliWrongTypeLine(
		std::string_view commandLabel,
		std::string_view target,
		const LocalCommands::LookupLvliWrongType& wrongType,
		bool includeResolvedEditorID)
	{
		std::string response =
			std::string(commandLabel) + " \"" + std::string(target) + "\" -> wrong-type form=" + FormatFormID(wrongType.formID);
		response += " type=";
		response += wrongType.formType;
		if (includeResolvedEditorID) {
			response += " editorID=\"";
			response += wrongType.editorID;
			response += "\"";
		}
		response += " source=\"";
		response += wrongType.source;
		response += "\"";
		return response;
	}

	std::string BuildLookupLvliHitLine(
		std::string_view commandLabel,
		std::string_view target,
		const LocalCommands::LookupLvliHit& hit,
		bool includeResolvedEditorID)
	{
		std::string response =
			std::string(commandLabel) + " \"" + std::string(target) + "\" -> hit form=" + FormatFormID(hit.formID);
		if (includeResolvedEditorID) {
			response += " editorID=\"";
			response += hit.editorID;
			response += "\"";
		}
		response += " entries=";
		response += std::to_string(hit.entries.size());
		response += " source=\"";
		response += hit.source;
		response += "\"";
		return response;
	}

	std::string FormatLookupLvliEntryLine(
		std::string_view commandLabel,
		const LocalCommands::LookupLvliEntry& entry)
	{
		std::string response = std::string(commandLabel) + " entry level=";
		response += std::to_string(entry.level);
		response += " count=";
		response += std::to_string(entry.count);
		response += " form=";
		response += FormatFormID(entry.formID);
		response += " type=";
		response += entry.formType;

		if (!entry.editorID.empty()) {
			response += " editorID=\"";
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
}

namespace LocalCommands
{
	std::vector<std::string> FormatLookupLvliResponse(
		std::string_view commandLabel,
		std::string_view target,
		std::size_t limit,
		const LookupLvliResult& result,
		bool includeResolvedEditorID)
	{
		std::vector<std::string> responseLines;
		switch (result.kind) {
		case LookupLvliResultKind::kMiss:
			responseLines.push_back(std::string(commandLabel) + " \"" + std::string(target) + "\" -> miss");
			return responseLines;
		case LookupLvliResultKind::kWrongType:
			responseLines.push_back(BuildLookupLvliWrongTypeLine(commandLabel, target, result.wrongType, includeResolvedEditorID));
			return responseLines;
		case LookupLvliResultKind::kHit:
			break;
		default:
			throw std::runtime_error("unknown lookup-lvli result kind");
		}

		const auto emittedCount = std::min(limit, result.hit.entries.size());
		responseLines.reserve(emittedCount + 2);
		responseLines.push_back(BuildLookupLvliHitLine(commandLabel, target, result.hit, includeResolvedEditorID));

		for (std::size_t index = 0; index < emittedCount; ++index) {
			responseLines.push_back(FormatLookupLvliEntryLine(commandLabel, result.hit.entries[index]));
		}

		if (result.hit.entries.size() > emittedCount) {
			responseLines.push_back(
				std::string(commandLabel) + " \"" + std::string(target) + "\" -> truncated to " + std::to_string(emittedCount) +
				" of " + std::to_string(result.hit.entries.size()) + " entries");
		}

		return responseLines;
	}
}
