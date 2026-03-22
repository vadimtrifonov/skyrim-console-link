#pragma once

#include "config/Settings.h"

#include <filesystem>

namespace Display
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath);
}
