#pragma once

#include <filesystem>

namespace Command
{
	struct ResolvedInboxPath
	{
		std::filesystem::path configuredPath;
		std::filesystem::path resolvedPath;
	};

	[[nodiscard]] ResolvedInboxPath ResolveInboxPath(
		const std::filesystem::path& configuredPath,
		const std::filesystem::path& logDirectory);
}
