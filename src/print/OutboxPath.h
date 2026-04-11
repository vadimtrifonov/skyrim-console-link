#pragma once

#include <filesystem>

namespace Print
{
	struct ResolvedOutboxPath
	{
		std::filesystem::path configuredPath;
		std::filesystem::path resolvedPath;
	};

	struct OutboxPathContext
	{
		std::filesystem::path configuredPath;
		std::filesystem::path logDirectory;
	};

	[[nodiscard]] ResolvedOutboxPath ResolveOutboxPath(const OutboxPathContext& context);
}
