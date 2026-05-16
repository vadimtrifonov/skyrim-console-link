#include "local_commands/lookup_lvli/LookupLvliCommand.h"

#include "local_commands/lookup_lvli/LookupLvliFormat.h"
#include "local_commands/lookup_lvli/LookupLvliInspect.h"
#include "local_commands/support/LocalCommandParsing.h"
#include "pch.h"

namespace
{
	constexpr std::size_t kDefaultLookupLvliLimit = 10;
	constexpr std::string_view kLookupLvliCommandLabel = "lookup-lvli";

	struct LookupLvliArguments
	{
		std::string editorID;
		std::size_t limit = kDefaultLookupLvliLimit;
	};

	using LocalCommands::ConsumeToken;
	using LocalCommands::ParsePositiveLimit;
	using LocalCommands::TrimAscii;

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
}

namespace LocalCommands
{
	std::vector<std::string> ExecuteLookupLvliCommand(std::string_view arguments)
	{
		const auto parsed = ParseLookupLvliArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-lvli <EditorID> [limit]" };
		}

		const auto* form = RE::TESForm::LookupByEditorID(parsed->editorID);
		return FormatLookupLvliResponse(
			kLookupLvliCommandLabel,
			parsed->editorID,
			parsed->limit,
			InspectLookupLvliForm(form),
			false);
	}

	std::vector<std::string> ExecuteLookupLvliCommand(
		std::string_view arguments,
		const LookupLvliResult& result)
	{
		const auto parsed = ParseLookupLvliArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-lvli <EditorID> [limit]" };
		}

		return FormatLookupLvliResponse(kLookupLvliCommandLabel, parsed->editorID, parsed->limit, result, false);
	}
}
