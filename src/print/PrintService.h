#pragma once

#include "config/Settings.h"

#include <filesystem>
#include <string_view>

namespace Print
{
	enum class ImmediatePrintResult
	{
		kPrinted,
		kUnavailable,
		kDisabled
	};

	void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath);
	[[nodiscard]] ImmediatePrintResult TryPrintImmediateLine(std::string_view line) noexcept;
}
