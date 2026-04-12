#pragma once

#include "config/Settings.h"

#include <filesystem>

namespace Inbox
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& inboxPath);
}
