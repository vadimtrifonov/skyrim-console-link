#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace LocalCommands
{
	struct LookupFormHit
	{
		std::uint32_t formID = 0;
		std::string formType;
		std::string editorID;
		std::string name;
		std::string source;
	};

	[[nodiscard]] std::vector<std::string> ExecuteLookupFormCommand(std::string_view arguments);
	[[nodiscard]] std::vector<std::string> ExecuteLookupFormCommand(
		std::string_view arguments,
		std::optional<LookupFormHit> hit);
}
