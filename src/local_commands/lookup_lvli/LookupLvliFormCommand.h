#pragma once

#include "local_commands/lookup_lvli/LookupLvli.h"

#include <string>
#include <string_view>
#include <vector>

namespace LocalCommands
{
	[[nodiscard]] std::vector<std::string> ExecuteLookupLvliFormCommand(std::string_view arguments);
	[[nodiscard]] std::vector<std::string> ExecuteLookupLvliFormCommand(
		std::string_view arguments,
		const LookupLvliResult& result);
}
