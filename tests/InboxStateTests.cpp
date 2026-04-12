#include "inbox/InboxState.h"

#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
	struct FileIdentityParts
	{
		std::uint32_t volumeSerialNumber;
		std::uint32_t fileIndexHigh;
		std::uint32_t fileIndexLow;
	};

	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	Inbox::InboxFileIdentity MakeIdentity(const FileIdentityParts parts)
	{
		Inbox::InboxFileIdentity identity{};
		identity.volumeSerialNumber = parts.volumeSerialNumber;
		identity.fileIndexHigh = parts.fileIndexHigh;
		identity.fileIndexLow = parts.fileIndexLow;
		return identity;
	}

	void TestWorkerStartState()
	{
		Inbox::WorkerStartState state;

		Expect(state.CanStart(false), "worker should be startable before startup");
		Expect(!state.CanStart(true), "worker should not start when the service is disabled");
		Expect(!state.Started(), "worker should not be marked started before success");

		state.MarkStarted();

		Expect(state.Started(), "worker should publish a started state after success");
		Expect(!state.CanStart(false), "worker should not start twice");
	}

	void TestFreshFileStartsAtBaselineInsteadOfReadingOldContent()
	{
		Inbox::InboxState state;
		const auto identity = MakeIdentity({ 1, 10, 100 });

		const auto observe = state.ObserveFile(identity, 8);
		Expect(!observe.shouldRead, "fresh inbox should baseline at EOF instead of reading old content");
		Expect(observe.readOffset == 8, "fresh inbox should baseline at the current file size");
		Expect(!state.HasPendingLines(), "fresh inbox should not queue pre-existing content");
	}

	void TestReplacementTreatsCurrentContentsAsNewInput()
	{
		Inbox::InboxState state;
		const auto originalIdentity = MakeIdentity({ 2, 20, 200 });
		const auto replacementIdentity = MakeIdentity({ 3, 30, 300 });

		Expect(!state.ObserveFile(originalIdentity, 0).shouldRead, "fresh inbox should establish a baseline before replacement");
		Expect(state.AppendChunk("old1\n").extractedCount == 1, "post-baseline append should queue one line");
		Expect(state.PendingLineCount() == 1, "queued line should be visible before replacement");

		const auto replacement = state.ObserveFile(replacementIdentity, 8);
		Expect(replacement.resetReason == Inbox::InboxResetReason::kReplaced, "replacement should reset state");
		Expect(replacement.droppedPendingLines == 1, "replacement should drop stale queued lines");
		Expect(replacement.readOffset == 0, "replacement should restart from the rewritten file head");
		Expect(replacement.shouldRead, "replacement contents should be treated as new live input");
		Expect(!state.HasPendingLines(), "replacement should empty the pending queue");
	}

	void TestTruncationTreatsCurrentContentsAsNewInput()
	{
		Inbox::InboxState state;
		const auto identity = MakeIdentity({ 4, 40, 400 });

		Expect(!state.ObserveFile(identity, 0).shouldRead, "fresh inbox should establish a baseline before truncation coverage");
		Expect(state.AppendChunk("one\ntwo\n").extractedCount == 2, "appended lines should queue after the baseline");
		Expect(state.PendingLineCount() == 2, "two lines should be pending before truncation");

		const auto truncated = state.ObserveFile(identity, 3);
		Expect(truncated.resetReason == Inbox::InboxResetReason::kTruncated, "smaller file size should be treated as truncation");
		Expect(truncated.droppedPendingLines == 2, "truncation should drop stale queued lines");
		Expect(truncated.readOffset == 0, "truncation should restart from the rewritten file head");
		Expect(truncated.shouldRead, "truncated file contents should be treated as new live input");
		Expect(!state.HasPendingLines(), "truncation should empty the pending queue");
	}

	void TestMissingFileResetsState()
	{
		Inbox::InboxState state;
		const auto identity = MakeIdentity({ 5, 50, 500 });

		Expect(!state.ObserveFile(identity, 0).shouldRead, "fresh inbox should establish a baseline before removal coverage");
		Expect(state.AppendChunk("cmd\n").extractedCount == 1, "appended line should queue before removal");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Inbox::InboxResetReason::kRemoved, "missing file should reset the inbox state");
		Expect(removed.droppedPendingLines == 1, "missing file should clear stale queued lines");
		Expect(!state.HasPendingLines(), "missing file should empty the pending queue");

		const auto removedAgain = state.ObserveMissingFile();
		Expect(removedAgain.resetReason == Inbox::InboxResetReason::kNone, "repeated missing-file observations should be idempotent");
	}

	void TestRecreatedFileTreatsCurrentContentsAsNewInput()
	{
		Inbox::InboxState state;
		const auto originalIdentity = MakeIdentity({ 10, 100, 1000 });
		const auto recreatedIdentity = MakeIdentity({ 11, 110, 1100 });

		Expect(!state.ObserveFile(originalIdentity, 0).shouldRead, "fresh inbox should establish a baseline before recreation coverage");
		Expect(state.AppendChunk("old\n").extractedCount == 1, "post-baseline append should queue one line before recreation");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Inbox::InboxResetReason::kRemoved, "missing file should reset state before recreation");
		Expect(removed.droppedPendingLines == 1, "missing file should drop pending lines from the removed file");

		const auto recreated = state.ObserveFile(recreatedIdentity, 9);
		Expect(recreated.readOffset == 0, "recreated inbox should restart from the file head");
		Expect(recreated.shouldRead, "recreated inbox contents should be treated as new live input");
	}

	void TestBoundedQueueRefillsFromBufferedFragment()
	{
		Inbox::InboxState state(Inbox::InboxStateLimits{
			.maxPendingLines = 2,
			.maxPendingFragmentBytes = static_cast<std::size_t>(8) * 1024 });
		const auto identity = MakeIdentity({ 6, 60, 600 });

		Expect(!state.ObserveFile(identity, 0).shouldRead, "fresh inbox should establish a baseline before queue-cap coverage");
		const auto appendResult = state.AppendChunk("a\nb\nc\n");
		Expect(appendResult.extractedCount == 2, "queue should stop extracting at the configured cap");
		Expect(appendResult.pendingCount == 2, "queue should report the configured cap");
		Expect(appendResult.queueAtCapacity, "queue should mark itself at capacity");

		const auto front = state.FrontLine();
		Expect(front.has_value(), "first queued line should be visible at the head");
		Expect(front->text == "a", "first queued line should remain at the head");

		Expect(state.PopFrontLine(front->sequence), "matching front sequence should pop the queued line");

		const auto refillResult = state.ObserveFile(identity, 6);
		Expect(refillResult.extractedCount == 1, "buffered lines should refill the queue once capacity is available");
		Expect(refillResult.pendingCount == 2, "queue should refill back to the configured cap");
		Expect(refillResult.queueAtCapacity, "queue should return to capacity after the refill");
		Expect(!refillResult.shouldRead, "refill from buffered data should not require another file read");

		const auto second = state.FrontLine();
		Expect(second.has_value(), "second line should be visible after one pop");
		Expect(second->text == "b", "second line should move to the head after one pop");

		Expect(state.PopFrontLine(second->sequence), "second front sequence should pop cleanly");
		const auto third = state.FrontLine();
		Expect(third.has_value(), "buffered line should become visible after the refill");
		Expect(third->text == "c", "buffered line should become visible after the refill");
	}

	void TestOversizedFragmentIsDiscarded()
	{
		Inbox::InboxState state(Inbox::InboxStateLimits{
			.maxPendingLines = 4,
			.maxPendingFragmentBytes = 4 });
		const auto identity = MakeIdentity({ 7, 70, 700 });

		Expect(!state.ObserveFile(identity, 0).shouldRead, "fresh inbox should establish a baseline before oversized-fragment coverage");
		const auto appendResult = state.AppendChunk("abcde");
		Expect(appendResult.oversizedFragmentBytes == 5, "oversized partial lines should be reported");
		Expect(appendResult.pendingFragmentBytes == 0, "oversized partial lines should be discarded");
		Expect(!state.HasPendingLines(), "oversized partial lines should not queue input");
	}

	void TestStaleFrontSequenceDoesNotPopReplacementLine()
	{
		Inbox::InboxState state;
		const auto originalIdentity = MakeIdentity({ 8, 80, 800 });
		const auto replacementIdentity = MakeIdentity({ 9, 90, 900 });

		Expect(!state.ObserveFile(originalIdentity, 0).shouldRead, "fresh inbox should establish a baseline before stale-sequence coverage");
		Expect(state.AppendChunk("cmd\n").extractedCount == 1, "original line should queue before replacement");
		const auto originalFront = state.FrontLine();
		Expect(originalFront.has_value(), "original front line should exist");
		const auto originalSequence = originalFront->sequence;

		const auto replacement = state.ObserveFile(replacementIdentity, 4);
		Expect(replacement.resetReason == Inbox::InboxResetReason::kReplaced, "replacement should reset state before stale-sequence coverage");
		Expect(replacement.shouldRead, "replacement contents should be treated as new live input");
		Expect(!state.PopFrontLine(originalSequence), "stale front sequence should not pop a later replacement line");
	}
}

void RunInboxStateTests()
{
	TestWorkerStartState();
	TestFreshFileStartsAtBaselineInsteadOfReadingOldContent();
	TestReplacementTreatsCurrentContentsAsNewInput();
	TestTruncationTreatsCurrentContentsAsNewInput();
	TestMissingFileResetsState();
	TestRecreatedFileTreatsCurrentContentsAsNewInput();
	TestBoundedQueueRefillsFromBufferedFragment();
	TestOversizedFragmentIsDiscarded();
	TestStaleFrontSequenceDoesNotPopReplacementLine();
}
