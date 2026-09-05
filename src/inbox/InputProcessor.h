#pragma once

#include "settings/Settings.h"

#include <filesystem>

namespace Inbox
{
	// Clear and establish this session's inbox without starting processing.
	void Initialize(const Settings::Values& settings, const std::filesystem::path& inboxPath);
	void StartProcessing();
}
