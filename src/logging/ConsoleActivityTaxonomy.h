#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace Logging
{
	enum class ConsoleActivityKind
	{
		kCapturedConsoleOutput,
		kCapturedConsoleInput,
		kSubmittedConsoleCommand,
		kLocalActivity
	};

	[[nodiscard]] inline std::string_view DescribeConsoleActivityChannel(const ConsoleActivityKind kind)
	{
		switch (kind) {
		case ConsoleActivityKind::kCapturedConsoleOutput:
			return "console-out";
		case ConsoleActivityKind::kCapturedConsoleInput:
			return "console-in";
		case ConsoleActivityKind::kSubmittedConsoleCommand:
			return "console-submit";
		case ConsoleActivityKind::kLocalActivity:
			return "local";
		}

		throw std::invalid_argument("Unknown console activity kind");
	}

	[[nodiscard]] inline std::string BuildConsoleActivityRecord(const ConsoleActivityKind kind, std::string_view line)
	{
		const auto channel = DescribeConsoleActivityChannel(kind);
		return "[" + std::string(channel) + "] " + std::string(line);
	}
}
