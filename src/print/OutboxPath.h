#pragma once

#include <filesystem>

namespace Print
{
	struct ResolvedOutboxPath
	{
		std::filesystem::path configuredPath;
		std::filesystem::path resolvedPath;
	};

	[[nodiscard]] ResolvedOutboxPath ResolveOutboxPath(
		const std::filesystem::path& configuredPath,
		const std::filesystem::path& logDirectory);
}
