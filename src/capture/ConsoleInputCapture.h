#pragma once

#include "config/Settings.h"

#include <string_view>

namespace Capture::Input
{
	enum class SubmitCommandResult
	{
		kSubmitted,
		kDeferred,
		kTerminalFailure
	};

	void Install(const Config::Settings& settings);
	[[nodiscard]] bool CanSubmitCommands() noexcept;
	[[nodiscard]] SubmitCommandResult SubmitCommand(std::string_view command) noexcept;
}
