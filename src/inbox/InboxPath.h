#pragma once

#include <filesystem>

namespace Inbox
{
	struct ResolvedPath
	{
		std::filesystem::path configuredPath;
		std::filesystem::path resolvedPath;
	};

	struct PathContext
	{
		std::filesystem::path configuredPath;
		std::filesystem::path logDirectory;
	};

	[[nodiscard]] ResolvedPath ResolvePath(const PathContext& context);
}
