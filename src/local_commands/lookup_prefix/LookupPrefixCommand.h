#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace LocalCommands
{
	struct LookupPrefixMatch
	{
		std::string editorID;
		std::uint32_t formID = 0;
		std::string formType;
		std::string name;
		std::string source;
	};

	[[nodiscard]] std::vector<std::string> ExecuteLookupPrefixCommand(std::string_view arguments);
	[[nodiscard]] std::vector<std::string> ExecuteLookupPrefixCommand(std::string_view arguments, std::span<const LookupPrefixMatch> matches);
}
