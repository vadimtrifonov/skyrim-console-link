#pragma once

#include <cstdint>

namespace Inbox
{
	struct InboxFileIdentity
	{
		std::uint32_t volumeSerialNumber = 0;
		std::uint32_t fileIndexHigh = 0;
		std::uint32_t fileIndexLow = 0;

		[[nodiscard]] bool operator==(const InboxFileIdentity& other) const noexcept
		{
			return volumeSerialNumber == other.volumeSerialNumber &&
			       fileIndexHigh == other.fileIndexHigh &&
			       fileIndexLow == other.fileIndexLow;
		}
	};

	struct FileObservation
	{
		InboxFileIdentity identity{};
		std::uint64_t size = 0;
		std::uint64_t lastWriteTime = 0;

		[[nodiscard]] bool operator==(const FileObservation& other) const noexcept
		{
			return identity == other.identity &&
			       size == other.size &&
			       lastWriteTime == other.lastWriteTime;
		}
	};
}
