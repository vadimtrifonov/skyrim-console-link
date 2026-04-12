#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace Print
{
	struct OutboxFileIdentity
	{
		std::uint32_t volumeSerialNumber = 0;
		std::uint32_t fileIndexHigh = 0;
		std::uint32_t fileIndexLow = 0;

		[[nodiscard]] bool operator==(const OutboxFileIdentity& other) const noexcept
		{
			return volumeSerialNumber == other.volumeSerialNumber &&
			       fileIndexHigh == other.fileIndexHigh &&
			       fileIndexLow == other.fileIndexLow;
		}
	};

	enum class OutboxResetReason
	{
		kNone,
		kRemoved,
		kTruncated,
		kReplaced
	};

	struct OutboxUpdateResult
	{
		OutboxResetReason resetReason{ OutboxResetReason::kNone };
		std::size_t droppedPendingLines = 0;
		std::size_t extractedCount = 0;
		std::size_t pendingCount = 0;
		std::size_t pendingFragmentBytes = 0;
		std::size_t oversizedFragmentBytes = 0;
		std::uint64_t readOffset = 0;
		bool queueAtCapacity = false;
		bool shouldRead = false;
	};

	struct OutboxStateLimits
	{
		std::size_t maxPendingLines = 256;
		std::size_t maxPendingFragmentBytes = 8 * 1024;
	};

	class OutboxState
	{
	public:
		explicit OutboxState(OutboxStateLimits limits = {});

		[[nodiscard]] OutboxUpdateResult ObserveMissingFile();
		[[nodiscard]] OutboxUpdateResult ObserveFile(const OutboxFileIdentity& identity, std::uint64_t fileSize);
		[[nodiscard]] OutboxUpdateResult AppendChunk(std::string_view chunk);

		[[nodiscard]] bool HasPendingLines() const noexcept;
		[[nodiscard]] std::size_t PendingLineCount() const noexcept;
		[[nodiscard]] std::size_t MaxPendingLines() const noexcept;
		[[nodiscard]] std::vector<std::string> DrainLines(std::size_t maxLines);

	private:
		[[nodiscard]] OutboxUpdateResult ResetState(OutboxResetReason reason);
		void ExtractCompleteLines(OutboxUpdateResult& result);

		std::size_t _maxPendingLines = 0;
		std::size_t _maxPendingFragmentBytes = 0;
		std::deque<std::string> _pendingLines;
		std::string _pendingFragment;
		OutboxFileIdentity _fileIdentity{};
		std::uint64_t _fileCursor = 0;
		bool _fileIdentityKnown = false;
	};
}
