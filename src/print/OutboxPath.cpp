#include "print/OutboxPath.h"

#include "pch.h"

#include <system_error>

namespace Print
{
	ResolvedOutboxPath ResolveOutboxPath(const OutboxPathContext& context)
	{
		ResolvedOutboxPath result{};
		result.configuredPath = context.configuredPath;
		result.resolvedPath = context.configuredPath;

		if (!result.resolvedPath.is_absolute()) {
			result.resolvedPath = context.logDirectory / result.resolvedPath;
		}

		std::error_code error;
		const auto absolutePath = std::filesystem::absolute(result.resolvedPath, error);
		if (!error) {
			result.resolvedPath = absolutePath;
		}

		return result;
	}
}
