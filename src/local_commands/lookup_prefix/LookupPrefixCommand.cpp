#include "local_commands/lookup_prefix/LookupPrefixCommand.h"

#include "local_commands/support/FormFormatting.h"
#include "local_commands/support/LocalCommandParsing.h"
#include "pch.h"

namespace
{
	constexpr std::size_t kDefaultLookupPrefixLimit = 10;

	using LocalCommands::ConsumeToken;
	using LocalCommands::DescribeFormType;
	using LocalCommands::FormatFormID;
	using LocalCommands::TrimAscii;

	struct LookupPrefixArguments
	{
		std::string prefix;
		std::size_t limit = kDefaultLookupPrefixLimit;
	};

	using LocalCommands::ParsePositiveLimit;

	[[nodiscard]] std::optional<LookupPrefixArguments> ParseLookupPrefixArguments(std::string_view arguments) noexcept
	{
		auto remaining = TrimAscii(arguments);
		const auto prefix = ConsumeToken(remaining);
		const auto limitToken = ConsumeToken(remaining);

		if (prefix.empty() || !remaining.empty()) {
			return std::nullopt;
		}

		LookupPrefixArguments parsed{};
		parsed.prefix = std::string(prefix);
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

	std::string FormatLookupMatchLine(const LocalCommands::LookupPrefixMatch& match)
	{
		std::string response = "lookup-prefix match editorID=\"";
		response += match.editorID;
		response += "\" form=";
		response += FormatFormID(match.formID);
		response += " type=";
		response += match.formType;

		if (!match.name.empty()) {
			response += " name=\"";
			response += match.name;
			response += "\"";
		}

		response += " source=\"";
		response += match.source;
		response += "\"";

		return response;
	}

	[[nodiscard]] std::vector<LocalCommands::LookupPrefixMatch> CollectLookupPrefixMatches(std::string_view prefix)
	{
		std::vector<LocalCommands::LookupPrefixMatch> matches;

		const auto& [map, lock] = RE::TESForm::GetAllFormsByEditorID();
		const RE::BSReadLockGuard guard{ lock };
		if (map == nullptr) {
			return matches;
		}

		for (const auto& entry : *map) {
			const auto editorID = std::string_view(entry.first);
			if (!editorID.starts_with(prefix)) {
				continue;
			}

			const auto* form = entry.second;
			if (form == nullptr) {
				continue;
			}

			LocalCommands::LookupPrefixMatch match{};
			match.editorID = std::string(editorID);
			match.formID = form->GetFormID();
			match.formType = std::string(DescribeFormType(form->GetFormType()));

			const auto* name = form->GetName();
			if (name != nullptr && name[0] != '\0') {
				match.name = name;
			}
			if (const auto* file = form->GetFile(); file != nullptr) {
				match.source = std::string(file->GetFilename());
			}

			matches.push_back(std::move(match));
		}

		return matches;
	}
}

namespace LocalCommands
{
	std::vector<std::string> ExecuteLookupPrefixCommand(std::string_view arguments)
	{
		const auto parsed = ParseLookupPrefixArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-prefix <prefix> [limit]" };
		}

		return ExecuteLookupPrefixCommand(arguments, CollectLookupPrefixMatches(parsed->prefix));
	}

	std::vector<std::string> ExecuteLookupPrefixCommand(
		std::string_view arguments,
		std::span<const LookupPrefixMatch> matches)
	{
		const auto parsed = ParseLookupPrefixArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-prefix <prefix> [limit]" };
		}

		std::vector<LookupPrefixMatch> orderedMatches(matches.begin(), matches.end());
		std::sort(
			orderedMatches.begin(),
			orderedMatches.end(),
			[](const LookupPrefixMatch& lhs, const LookupPrefixMatch& rhs) {
				return lhs.editorID < rhs.editorID;
			});

		const auto emittedCount = std::min(parsed->limit, orderedMatches.size());
		std::vector<std::string> responseLines;
		responseLines.reserve(emittedCount + 2);

		if (orderedMatches.empty()) {
			responseLines.push_back("lookup-prefix \"" + parsed->prefix + "\" -> miss");
			return responseLines;
		}

		responseLines.push_back(
			"lookup-prefix \"" + parsed->prefix + "\" -> " + std::to_string(orderedMatches.size()) + " match(es)");

		for (std::size_t index = 0; index < emittedCount; ++index) {
			responseLines.push_back(FormatLookupMatchLine(orderedMatches[index]));
		}

		if (orderedMatches.size() > emittedCount) {
			responseLines.push_back(
				"lookup-prefix \"" + parsed->prefix + "\" -> truncated to " + std::to_string(emittedCount) +
				" of " + std::to_string(orderedMatches.size()) + " match(es)");
		}

		return responseLines;
	}
}
