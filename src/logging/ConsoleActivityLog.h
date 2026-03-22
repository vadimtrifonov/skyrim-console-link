#pragma once

#include "config/Settings.h"

#include <string_view>

namespace Logging
{
	void InitializeConsoleActivityLog(const Config::Settings& settings);
	void LogConsoleInputLine(std::string_view line) noexcept;
	void LogConsoleOutputLine(std::string_view line) noexcept;
	void LogConsoleSubmitLine(std::string_view line) noexcept;
	void LogDisplayLine(std::string_view line) noexcept;
}
