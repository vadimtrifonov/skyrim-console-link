#pragma once

#include "local_commands/lookup_lvli/LookupLvli.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace LocalCommands
{
	[[nodiscard]] std::vector<std::string> FormatLookupLvliResponse(
		std::string_view commandLabel,
		std::string_view target,
		std::size_t limit,
		const LookupLvliResult& result,
		bool includeResolvedEditorID);
}
