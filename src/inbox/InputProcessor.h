#pragma once

#include "settings/Settings.h"

#include <filesystem>

namespace Inbox
{
	void Initialize(const Settings::Values& settings, const std::filesystem::path& inboxPath);
}
