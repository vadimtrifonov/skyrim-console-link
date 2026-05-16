#include "local_commands/lookup_lvli/LookupLvliFormCommand.h"

#include "local_commands/lookup_lvli/LookupLvliFormat.h"
#include "local_commands/lookup_lvli/LookupLvliInspect.h"
#include "local_commands/support/FormKey.h"
#include "local_commands/support/LocalCommandParsing.h"
#include "pch.h"

#include <RE/T/TESDataHandler.h>

#include <stdexcept>

namespace
{
	constexpr std::size_t kDefaultLookupLvliFormLimit = 10;
	constexpr std::string_view kLookupLvliFormCommandLabel = "lookup-lvli-form";

	struct LookupLvliFormArguments
	{
		std::string formKey;
		std::size_t limit = kDefaultLookupLvliFormLimit;
	};

	using LocalCommands::ParsePositiveLimit;
	using LocalCommands::TrimAscii;

	[[nodiscard]] std::optional<LookupLvliFormArguments> ParseLookupLvliFormArguments(std::string_view arguments) noexcept
	{
		const auto trimmedArguments = TrimAscii(arguments);
		if (trimmedArguments.empty()) {
			return std::nullopt;
		}

		LookupLvliFormArguments parsed{};
		const auto lastWhitespace = trimmedArguments.find_last_of(" \t");
		if (lastWhitespace != std::string_view::npos) {
			const auto formKeyCandidate = TrimAscii(trimmedArguments.substr(0, lastWhitespace));
			const auto limitToken = TrimAscii(trimmedArguments.substr(lastWhitespace + 1));
			const auto limit = ParsePositiveLimit(limitToken);
			if (limit.has_value()) {
				if (formKeyCandidate.empty() || !LocalCommands::ParseFormKey(formKeyCandidate).has_value()) {
					return std::nullopt;
				}

				parsed.formKey = std::string(formKeyCandidate);
				parsed.limit = *limit;
				return parsed;
			}
		}

		if (!LocalCommands::ParseFormKey(trimmedArguments).has_value()) {
			return std::nullopt;
		}

		parsed.formKey = std::string(trimmedArguments);
		return parsed;
	}
}

namespace LocalCommands
{
	std::vector<std::string> ExecuteLookupLvliFormCommand(std::string_view arguments)
	{
		const auto parsed = ParseLookupLvliFormArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]" };
		}

		const auto parsedKey = ParseFormKey(parsed->formKey);
		if (!parsedKey.has_value()) {
			throw std::runtime_error("cdbg.lookup-lvli-form received an invalid parsed form key");
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (dataHandler == nullptr) {
			throw std::runtime_error("cdbg.lookup-lvli-form could not access TESDataHandler");
		}

		const auto* form = LookupFormByFormKey(*parsedKey, *dataHandler);
		return FormatLookupLvliResponse(
			kLookupLvliFormCommandLabel,
			parsed->formKey,
			parsed->limit,
			InspectLookupLvliForm(form),
			true);
	}

	std::vector<std::string> ExecuteLookupLvliFormCommand(
		std::string_view arguments,
		const LookupLvliResult& result)
	{
		const auto parsed = ParseLookupLvliFormArguments(arguments);
		if (!parsed.has_value()) {
			return { "usage: cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]" };
		}

		return FormatLookupLvliResponse(kLookupLvliFormCommandLabel, parsed->formKey, parsed->limit, result, true);
	}
}
