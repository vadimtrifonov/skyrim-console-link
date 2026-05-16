#include "local_commands/support/FormKey.h"

#include "pch.h"

#include <RE/T/TESDataHandler.h>

#include <charconv>

namespace
{
	constexpr std::uint32_t kMaxPluginLocalFormID = 0x00FFFFFF;
}

namespace LocalCommands
{
	std::optional<FormKey> ParseFormKey(std::string_view token) noexcept
	{
		const auto separator = token.find('|');
		if (separator == std::string_view::npos || separator == 0 || separator == token.size() - 1) {
			return std::nullopt;
		}

		if (token.find('|', separator + 1) != std::string_view::npos) {
			return std::nullopt;
		}

		const auto plugin = token.substr(0, separator);
		const auto formToken = token.substr(separator + 1);
		std::uint32_t formID = 0;
		const auto [ptr, ec] = std::from_chars(formToken.data(), formToken.data() + formToken.size(), formID, 16);
		if (ec != std::errc() || ptr != formToken.data() + formToken.size() || formID > kMaxPluginLocalFormID) {
			return std::nullopt;
		}

		return FormKey{
			.plugin = std::string(plugin),
			.formID = formID,
		};
	}

	RE::TESForm* LookupFormByFormKey(const FormKey& formKey, RE::TESDataHandler& dataHandler) noexcept
	{
		return dataHandler.LookupForm(formKey.formID, formKey.plugin);
	}
}
