#pragma once

#include "config/Settings.h"

#include <optional>
#include <string>
#include <string_view>

namespace Logging
{
	[[nodiscard]] std::optional<std::string> InitializeConsoleActivityLog(const Config::Settings& settings);
	void                                     LogConsoleInputLine(std::string_view line) noexcept;
	void                                     LogConsoleOutputLine(std::string_view line) noexcept;
	void                                     LogConsoleSubmitLine(std::string_view line) noexcept;
	void                                     LogLocalActivityLine(std::string_view line) noexcept;
	void                                     LogConsolePrintLine(std::string_view line) noexcept;
}
