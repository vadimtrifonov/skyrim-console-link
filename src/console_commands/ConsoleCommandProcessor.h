#pragma once

#include "settings/Settings.h"

#include <string_view>

namespace ConsoleCommands
{
	enum class ProcessResult
	{
		kHandled,
		kRejected
	};

	void Initialize(const Settings::Values& settings) noexcept;
	[[nodiscard]] ProcessResult Process(std::string_view line) noexcept;
}
