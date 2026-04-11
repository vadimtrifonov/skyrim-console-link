#include "display/OutboxPath.h"

#include "pch.h"

#include <system_error>

namespace Display
{
	ResolvedOutboxPath ResolveOutboxPath(
		const std::filesystem::path& configuredPath,
		const std::filesystem::path& logDirectory)
	{
		ResolvedOutboxPath result{};
		result.configuredPath = configuredPath;
		result.resolvedPath = configuredPath;

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

