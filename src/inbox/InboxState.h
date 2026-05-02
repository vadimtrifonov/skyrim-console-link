#pragma once

#include "inbox/InboxFileObservation.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace Inbox
{
	enum class InboxResetReason
	{
		kNone,
		kRemoved,
		kTruncated,
		kReplaced,
		kOverwritten
	};

	enum class InboxReadMode
	{
		kNone,
		kAppendCandidate,
		kRewrite
	};

	enum class InboxTrackingState
	{
		kNotReady,
		kReadyNoTrackedFile,
		kTrackingAppend,
		kRewritePending
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
		InboxReadMode readMode = InboxReadMode::kNone;
		bool queueAtCapacity = false;
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
		[[nodiscard]] InboxUpdateResult ObserveFile(const FileObservation& observation);
		[[nodiscard]] InboxUpdateResult ObserveFile(const InboxFileIdentity& identity, std::uint64_t fileSize);
		[[nodiscard]] InboxUpdateResult AppendObservedContents(std::uint64_t appendOffset, std::string_view observedContents);
		[[nodiscard]] InboxUpdateResult AppendChunk(std::string_view chunk);
		[[nodiscard]] InboxUpdateResult AcceptRewriteContents(const FileObservation& stableObservation, std::string_view contents);

		[[nodiscard]] bool HasPendingLines() const noexcept;
		[[nodiscard]] std::size_t PendingLineCount() const noexcept;
		[[nodiscard]] std::optional<PendingInputLine> FrontLine() const;
		bool PopFrontLine(std::uint64_t sequence);

		[[nodiscard]] std::size_t MaxPendingLines() const noexcept;

	private:
		[[nodiscard]] InboxUpdateResult ResetState(InboxResetReason reason, std::uint64_t baselineOffset);
		[[nodiscard]] InboxUpdateResult EnterRewrite(InboxResetReason reason);
		void ExtractCompleteLines(InboxUpdateResult& result);

		std::size_t _maxPendingLines = 0;
		std::size_t _maxPendingFragmentBytes = 0;
		std::deque<PendingInputLine> _pendingLines;
		std::string _pendingFragment;
		std::string _acceptedContents;
		InboxFileIdentity _fileIdentity{};
		std::uint64_t _fileCursor = 0;
		std::uint64_t _fileLastWriteTime = 0;
		std::uint64_t _nextSequence = 1;
		InboxTrackingState _trackingState = InboxTrackingState::kNotReady;
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
