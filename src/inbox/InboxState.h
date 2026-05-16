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
	enum class ResetReason
	{
		kNone,
		kRemoved,
		kTruncated,
		kReplaced,
		kOverwritten
	};

	enum class ReadMode
	{
		kNone,
		kAppendCandidate,
		kRewrite
	};

	enum class TrackingState
	{
		kNotReady,
		kReadyNoTrackedFile,
		kTrackingAppend,
		kRewritePending
	};

	struct UpdateResult
	{
		ResetReason resetReason{ ResetReason::kNone };
		std::size_t droppedPendingLines = 0;
		std::size_t extractedCount = 0;
		std::size_t pendingCount = 0;
		std::size_t pendingFragmentBytes = 0;
		std::size_t oversizedFragmentBytes = 0;
		std::uint64_t readOffset = 0;
		ReadMode readMode = ReadMode::kNone;
		bool queueAtCapacity = false;
	};

	struct PendingLine
	{
		std::uint64_t sequence = 0;
		std::string text;
	};

	struct StateLimits
	{
		std::size_t maxPendingLines = 256;
		std::size_t maxPendingFragmentBytes = 8 * 1024;
	};

	class State
	{
	public:
		explicit State(StateLimits limits = {});

		void ResetForNewSession() noexcept;
		[[nodiscard]] UpdateResult ObserveMissingFile();
		[[nodiscard]] UpdateResult ObserveFile(const FileObservation& observation);
		[[nodiscard]] UpdateResult ObserveFile(const FileIdentity& identity, std::uint64_t fileSize);
		[[nodiscard]] UpdateResult AppendObservedContents(std::uint64_t appendOffset, std::string_view observedContents);
		[[nodiscard]] UpdateResult AppendChunk(std::string_view chunk);
		[[nodiscard]] UpdateResult AcceptRewriteContents(const FileObservation& stableObservation, std::string_view contents);

		[[nodiscard]] bool HasPendingLines() const noexcept;
		[[nodiscard]] std::size_t PendingLineCount() const noexcept;
		[[nodiscard]] std::optional<PendingLine> FrontLine() const;
		bool PopFrontLine(std::uint64_t sequence);

		[[nodiscard]] std::size_t MaxPendingLines() const noexcept;

	private:
		[[nodiscard]] UpdateResult ResetState(ResetReason reason, std::uint64_t baselineOffset);
		[[nodiscard]] UpdateResult EnterRewrite(ResetReason reason);
		void ExtractCompleteLines(UpdateResult& result);

		std::size_t _maxPendingLines = 0;
		std::size_t _maxPendingFragmentBytes = 0;
		std::deque<PendingLine> _pendingLines;
		std::string _pendingFragment;
		std::string _acceptedContents;
		FileIdentity _fileIdentity{};
		std::uint64_t _fileCursor = 0;
		std::uint64_t _fileLastWriteTime = 0;
		std::uint64_t _nextSequence = 1;
		TrackingState _trackingState = TrackingState::kNotReady;
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
