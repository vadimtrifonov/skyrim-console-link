#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

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

	enum class InboxResetReason
	{
		kNone,
		kRemoved,
		kTruncated,
		kReplaced
	};

	struct InboxUpdateResult
	{
		InboxResetReason resetReason{ InboxResetReason::kNone };
		std::size_t droppedPendingLines = 0;
		std::size_t extractedCount = 0;
		std::size_t pendingCount = 0;
		std::size_t pendingFragmentBytes = 0;
		std::size_t oversizedFragmentBytes = 0;
		std::uint64_t readOffset = 0;
		bool queueAtCapacity = false;
		bool shouldRead = false;
	};

	struct PendingInputLine
	{
		std::uint64_t sequence = 0;
		std::string text;
	};

	struct InboxStateLimits
	{
		std::size_t maxPendingLines = 256;
		std::size_t maxPendingFragmentBytes = 8 * 1024;
	};

	class InboxState
	{
	public:
		explicit InboxState(InboxStateLimits limits = {});

		void ResetForNewSession() noexcept;
		[[nodiscard]] InboxUpdateResult ObserveMissingFile();
		[[nodiscard]] InboxUpdateResult ObserveFile(const InboxFileIdentity& identity, std::uint64_t fileSize);
		[[nodiscard]] InboxUpdateResult AppendChunk(std::string_view chunk);

		[[nodiscard]] bool HasPendingLines() const noexcept;
		[[nodiscard]] std::size_t PendingLineCount() const noexcept;
		[[nodiscard]] std::optional<PendingInputLine> FrontLine() const;
		bool PopFrontLine(std::uint64_t sequence);

		[[nodiscard]] std::size_t MaxPendingLines() const noexcept;

	private:
		[[nodiscard]] InboxUpdateResult ResetState(InboxResetReason reason, std::uint64_t baselineOffset);
		void ExtractCompleteLines(InboxUpdateResult& result);

		std::size_t _maxPendingLines = 0;
		std::size_t _maxPendingFragmentBytes = 0;
		std::deque<PendingInputLine> _pendingLines;
		std::string _pendingFragment;
		InboxFileIdentity _fileIdentity{};
		std::uint64_t _fileCursor = 0;
		std::uint64_t _nextSequence = 1;
		bool _fileIdentityKnown = false;
		bool _baselineEstablished = false;
	};

	class WorkerStartState
	{
	public:
		[[nodiscard]] bool CanStart(bool serviceDisabled) const noexcept
		{
			return !serviceDisabled && !_started;
		}

		void MarkStarted() noexcept
		{
			_started = true;
		}

		[[nodiscard]] bool Started() const noexcept
		{
			return _started;
		}

	private:
		bool _started = false;
	};
}
