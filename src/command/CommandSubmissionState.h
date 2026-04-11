#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace Command
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
		std::size_t      droppedPendingCommands = 0;
		std::size_t      extractedCount = 0;
		std::size_t      pendingCount = 0;
		std::size_t      pendingFragmentBytes = 0;
		std::size_t      oversizedFragmentBytes = 0;
		std::uint64_t    readOffset = 0;
		bool             queueAtCapacity = false;
		bool             shouldRead = false;
	};

	struct PendingCommand
	{
		std::uint64_t sequence = 0;
		std::string   text;
	};

	class InboxState
	{
	public:
		explicit InboxState(
			std::size_t maxPendingCommands = 256,
			std::size_t maxPendingFragmentBytes = 8 * 1024);

		[[nodiscard]] InboxUpdateResult ObserveMissingFile();
		[[nodiscard]] InboxUpdateResult ObserveFile(const InboxFileIdentity& identity, std::uint64_t fileSize);
		[[nodiscard]] InboxUpdateResult AppendChunk(std::string_view chunk);

		[[nodiscard]] bool                          HasPendingCommands() const noexcept;
		[[nodiscard]] std::size_t                   PendingCommandCount() const noexcept;
		[[nodiscard]] std::optional<PendingCommand> FrontCommand() const;
		bool                                        PopFrontCommand(std::uint64_t sequence);

		[[nodiscard]] std::size_t MaxPendingCommands() const noexcept;

	private:
		[[nodiscard]] InboxUpdateResult ResetState(InboxResetReason reason);
		void                            ExtractCompleteLines(InboxUpdateResult& result);

		std::size_t                _maxPendingCommands = 0;
		std::size_t                _maxPendingFragmentBytes = 0;
		std::deque<PendingCommand> _pendingCommands;
		std::string                _pendingFragment;
		InboxFileIdentity          _fileIdentity{};
		std::uint64_t              _fileCursor = 0;
		std::uint64_t              _nextCommandSequence = 1;
		bool                       _fileIdentityKnown = false;
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
