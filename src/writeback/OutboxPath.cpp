#include "writeback/OutboxPath.h"

#include "config/Settings.h"
#include "pch.h"

namespace
{
	constexpr auto kLegacyDefaultOutboxPath = L"Data/SKSE/Plugins/ConsoleDebugger.outbox.txt";
}

namespace WriteBack
{
	ResolvedOutboxPath ResolveOutboxPath(
		const std::filesystem::path& configuredPath,
		const std::filesystem::path& logDirectory)
	{
		ResolvedOutboxPath result{};
		result.configuredPath = configuredPath;
		result.resolvedPath = configuredPath;

		if (configuredPath.lexically_normal() == std::filesystem::path(kLegacyDefaultOutboxPath).lexically_normal()) {
			result.resolvedPath = Config::GetDefaultOutboxPath();
			result.remappedLegacyDefault = true;
		}

		if (!result.resolvedPath.is_absolute()) {
			result.resolvedPath = logDirectory / result.resolvedPath;
		}

		std::error_code error;
		const auto      absolutePath = std::filesystem::absolute(result.resolvedPath, error);
		if (!error) {
			result.resolvedPath = absolutePath;
		}

		return result;
	}
}
