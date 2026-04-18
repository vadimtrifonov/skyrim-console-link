#include "local_commands/support/FormKey.h"

#include "pch.h"

#include <RE/T/TESDataHandler.h>
#include <RE/T/TESFile.h>
#include <RE/T/TESForm.h>

#include <charconv>

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
		if (ec != std::errc() || ptr != formToken.data() + formToken.size()) {
			return std::nullopt;
		}

		return FormKey{
			.plugin = std::string(plugin),
			.formID = formID,
		};
	}

	std::optional<std::uint32_t> BuildRuntimeFormID(PluginLoadAddress loadAddress, std::uint32_t localFormID) noexcept
	{
		if (loadAddress.compileIndex == 0xFF) {
			return std::nullopt;
		}

		std::uint32_t runtimeFormID = static_cast<std::uint32_t>(loadAddress.compileIndex) << 24;
		if (loadAddress.isLight) {
			runtimeFormID += static_cast<std::uint32_t>(loadAddress.smallFileCompileIndex) << 12;
		}
		runtimeFormID += localFormID;
		return runtimeFormID;
	}

	RE::TESForm* LookupFormByFormKey(const FormKey& formKey, RE::TESDataHandler& dataHandler) noexcept
	{
		const auto* file = dataHandler.LookupModByName(formKey.plugin);
		if (file == nullptr) {
			return nullptr;
		}

		const auto runtimeFormID = BuildRuntimeFormID(
			PluginLoadAddress{
				.compileIndex = file->GetCompileIndex(),
				.smallFileCompileIndex = file->GetSmallFileCompileIndex(),
				.isLight = file->IsLight(),
			},
			formKey.formID);
		if (!runtimeFormID.has_value()) {
			return nullptr;
		}

		// RE::TESDataHandler::LookupForm() follows the stock VR path, which drops the
		// light-plugin subindex. Compose the runtime ID here so plugin-qualified lookups
		// still resolve FE-prefixed light-plugin records.
		return RE::TESForm::LookupByID(*runtimeFormID);
	}
}
