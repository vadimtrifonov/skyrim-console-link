#pragma once

#include <filesystem>

namespace Command
{
	struct ResolvedInboxPath
	{
		std::filesystem::path configuredPath;
		std::filesystem::path resolvedPath;
	};

	struct InboxPathContext
	{
		std::filesystem::path configuredPath;
		std::filesystem::path logDirectory;
	};

	[[nodiscard]] ResolvedInboxPath ResolveInboxPath(const InboxPathContext& context);
}
