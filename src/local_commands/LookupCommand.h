#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace LocalCommands
{
	struct LookupHit
	{
		std::uint32_t formID = 0;
		std::string formType;
		std::string name;
		std::string source;
	};

	[[nodiscard]] std::vector<std::string> ExecuteLookupCommand(std::string_view arguments);
	[[nodiscard]] std::vector<std::string> ExecuteLookupCommand(
		std::string_view arguments,
		std::optional<LookupHit> hit);
}
