#pragma once

#include "config/Settings.h"

#include <string_view>

namespace Logging
{
	void InitializeConsoleActivityLog(const Config::Settings& settings);
	void LogWriteBackLine(std::string_view line);
}
