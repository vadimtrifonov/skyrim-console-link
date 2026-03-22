#pragma once

#include <filesystem>

namespace WriteBack
{
	struct ResolvedOutboxPath
	{
		std::filesystem::path configuredPath;
		std::filesystem::path resolvedPath;
		bool                  remappedLegacyDefault = false;
	};

	[[nodiscard]] ResolvedOutboxPath ResolveOutboxPath(
		const std::filesystem::path& configuredPath,
		const std::filesystem::path& logDirectory);
}
