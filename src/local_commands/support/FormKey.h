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

	[[nodiscard]] std::optional<FormKey> ParseFormKey(std::string_view token) noexcept;
	[[nodiscard]] RE::TESForm* LookupFormByFormKey(const FormKey& formKey, RE::TESDataHandler& dataHandler) noexcept;
}
