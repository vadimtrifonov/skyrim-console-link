#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace Logging
{
	[[nodiscard]] inline std::string_view GetConsoleActivityLogFilename() noexcept
	{
		return "ConsoleLink.activity.log";
	}

	[[nodiscard]] std::optional<std::string> InitializeConsoleActivityLog();
	void LogConsoleInputLine(std::string_view line) noexcept;
	void LogConsoleOutputLine(std::string_view line) noexcept;
	void LogConsoleSubmitLine(std::string_view line) noexcept;
	void LogLocalActivityLine(std::string_view line) noexcept;
}
