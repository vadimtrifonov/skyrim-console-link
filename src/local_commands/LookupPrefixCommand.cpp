#include "local_commands/LookupPrefixCommand.h"

#include "pch.h"

#include <cctype>
#include <charconv>
#include <iomanip>
#include <sstream>

namespace
{
	constexpr std::size_t kDefaultLookupPrefixLimit = 10;

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

	struct LookupPrefixArguments
	{
		std::string prefix;
		std::size_t limit = kDefaultLookupPrefixLimit;
	};

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
		std::string response = "lookup-prefix match editor=\"";
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
		[[maybe_unused]] const RE::BSReadWriteLock guard{ lock };
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
