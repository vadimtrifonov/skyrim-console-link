#pragma once

#include "settings/Settings.h"

#include <filesystem>

namespace Runtime
{
	struct RuntimePaths
	{
		std::filesystem::path inboxPath;
	};

	void Initialize(const Settings::Values& settings, const RuntimePaths& paths);
}
