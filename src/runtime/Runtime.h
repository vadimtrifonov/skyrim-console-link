#pragma once

#include "config/Settings.h"

#include <filesystem>

namespace Runtime
{
	void Initialize(
		const Config::Settings&      settings,
		const std::filesystem::path& outboxPath,
		const std::filesystem::path& inboxPath);
}
