#pragma once

#include "config/Settings.h"

#include <filesystem>

namespace Runtime
{
	struct RuntimePaths
	{
		std::filesystem::path outboxPath;
		std::filesystem::path inboxPath;
	};

	void Initialize(const Config::Settings& settings, const RuntimePaths& paths);
}
