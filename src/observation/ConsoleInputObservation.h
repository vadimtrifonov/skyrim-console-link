#pragma once

#include "settings/Settings.h"

#include <string_view>

namespace Observation::Input
{
	enum class SubmitCommandResult
	{
		kSubmitted,
		kDeferred,
		kTerminalFailure
	};

	void Install(const Settings::Values& settings);
	[[nodiscard]] bool CanSubmitCommands() noexcept;
	[[nodiscard]] SubmitCommandResult SubmitCommand(std::string_view command) noexcept;
}
