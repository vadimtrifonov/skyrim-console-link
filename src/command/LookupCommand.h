#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Command
{
	[[nodiscard]] std::vector<std::string> ExecuteLookupCommand(std::string_view arguments);
}
