#pragma once

#include "config/Settings.h"

#include <filesystem>

namespace WriteBack
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath);
}
