#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace RE
{
	class TESDataHandler;
	class TESForm;
}

namespace LocalCommands
{
	struct FormKey
	{
		std::string plugin;
		std::uint32_t formID = 0;
	};

	struct PluginLoadAddress
	{
		std::uint8_t compileIndex = 0xFF;
		std::uint16_t smallFileCompileIndex = 0;
		bool isLight = false;
	};

	[[nodiscard]] std::optional<FormKey> ParseFormKey(std::string_view token) noexcept;
	[[nodiscard]] std::optional<std::uint32_t> BuildRuntimeFormID(PluginLoadAddress loadAddress, std::uint32_t localFormID) noexcept;
	[[nodiscard]] RE::TESForm* LookupFormByFormKey(const FormKey& formKey, RE::TESDataHandler& dataHandler) noexcept;
}
