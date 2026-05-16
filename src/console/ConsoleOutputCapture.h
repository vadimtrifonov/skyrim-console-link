#pragma once

#include "settings/Settings.h"

#include <cstddef>

namespace Console::OutputCapture
{
	[[nodiscard]] std::size_t RequiredTrampolineSize(const Settings::Values& settings) noexcept;
	void Install(const Settings::Values& settings);
}
