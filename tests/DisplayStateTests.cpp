#include "display/DisplayState.h"

#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	Display::OutboxFileIdentity MakeIdentity(
		const std::uint32_t volumeSerialNumber,
		const std::uint32_t fileIndexHigh,
		const std::uint32_t fileIndexLow)
	{
		Display::OutboxFileIdentity identity{};
		identity.volumeSerialNumber = volumeSerialNumber;
		identity.fileIndexHigh = fileIndexHigh;
		identity.fileIndexLow = fileIndexLow;
		return identity;
	}

	void TestOutboxReplacementResetsFromNewHead()
	{
		Display::OutboxState state;
		const auto          originalIdentity = MakeIdentity(1, 10, 100);
		const auto          replacementIdentity = MakeIdentity(2, 20, 200);

		const auto observeOriginal = state.ObserveFile(originalIdentity, 9);
		Expect(observeOriginal.shouldRead, "fresh outbox should read from offset zero");
		Expect(observeOriginal.readOffset == 0, "fresh outbox should start at the file head");

		const auto appendOriginal = state.AppendChunk("line1\n");
		Expect(appendOriginal.extractedCount == 1, "original outbox content should queue one display line");
		Expect(state.PendingLineCount() == 1, "original outbox line should remain pending");

		const auto observeReplacement = state.ObserveFile(replacementIdentity, 9);
		Expect(observeReplacement.resetReason == Display::OutboxResetReason::kReplaced, "replacement should reset the outbox state");
		Expect(observeReplacement.droppedPendingLines == 1, "replacement should drop stale queued display lines");
		Expect(observeReplacement.readOffset == 0, "replacement should reread from the new file head");
		Expect(observeReplacement.shouldRead, "replacement should request a read from the new file head");
		Expect(state.PendingLineCount() == 0, "replacement should clear the pending display queue");
	}

	void TestOutboxTruncationResetsQueuedLines()
	{
		Display::OutboxState state;
		const auto          identity = MakeIdentity(3, 30, 300);

		const auto observeOriginal = state.ObserveFile(identity, 10);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before truncation coverage");
		const auto appendOriginal = state.AppendChunk("one\ntwo\n");
		Expect(appendOriginal.extractedCount == 2, "original outbox content should queue two display lines");
		Expect(state.PendingLineCount() == 2, "two display lines should be pending before truncation");

		const auto observeTruncated = state.ObserveFile(identity, 3);
		Expect(observeTruncated.resetReason == Display::OutboxResetReason::kTruncated, "smaller file size should be treated as truncation");
		Expect(observeTruncated.droppedPendingLines == 2, "truncation should clear stale queued display lines");
		Expect(observeTruncated.readOffset == 0, "truncation should restart reading from the file head");
		Expect(observeTruncated.shouldRead, "truncation should request a reread from the new file head");
		Expect(state.PendingLineCount() == 0, "truncation should empty the pending display queue");
	}

	void TestOutboxRemovalResetsState()
	{
		Display::OutboxState state;
		const auto          identity = MakeIdentity(4, 40, 400);

		const auto observeOriginal = state.ObserveFile(identity, 5);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before removal coverage");
		const auto appendOriginal = state.AppendChunk("msg\n");
		Expect(appendOriginal.extractedCount == 1, "original outbox content should queue one display line");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Display::OutboxResetReason::kRemoved, "missing file should reset the outbox state");
		Expect(removed.droppedPendingLines == 1, "missing file should clear stale queued display lines");
		Expect(!state.HasPendingLines(), "missing file should empty the pending display queue");

		const auto removedAgain = state.ObserveMissingFile();
		Expect(removedAgain.resetReason == Display::OutboxResetReason::kNone, "repeated missing-file observations should be idempotent");
	}

	void TestBoundedQueueRefillsFromBufferedFragment()
	{
		Display::OutboxState state(2, 8 * 1024);
		const auto          identity = MakeIdentity(5, 50, 500);

		const auto observeOriginal = state.ObserveFile(identity, 6);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before queue-cap coverage");
		const auto appendResult = state.AppendChunk("a\nb\nc\n");
		Expect(appendResult.extractedCount == 2, "queue should stop extracting at the configured cap");
		Expect(appendResult.pendingCount == 2, "queue should report the configured cap");
		Expect(appendResult.queueAtCapacity, "queue should mark itself at capacity");

		const auto drained = state.DrainLines(1);
		Expect(drained.size() == 1, "draining one display line should return exactly one line");
		Expect(drained.front() == "a", "the first drained display line should be the queue head");

		const auto refillResult = state.ObserveFile(identity, 6);
		Expect(refillResult.extractedCount == 1, "buffered lines should refill the display queue once capacity is available");
		Expect(refillResult.pendingCount == 2, "display queue should refill back to the configured cap");
		Expect(refillResult.queueAtCapacity, "display queue should return to capacity after the refill");
		Expect(!refillResult.shouldRead, "refill from buffered data should not require another file read");

		const auto drainedRemaining = state.DrainLines(2);
		Expect(drainedRemaining.size() == 2, "the remaining buffered display lines should now be drainable");
		Expect(drainedRemaining[0] == "b", "second display line should move to the head after one drain");
		Expect(drainedRemaining[1] == "c", "buffered display line should become visible after the refill");
	}

	void TestOversizedFragmentIsDiscarded()
	{
		Display::OutboxState state(4, 4);
		const auto          identity = MakeIdentity(6, 60, 600);

		const auto observeOriginal = state.ObserveFile(identity, 5);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before oversized-fragment coverage");
		const auto appendResult = state.AppendChunk("abcde");
		Expect(appendResult.oversizedFragmentBytes == 5, "oversized partial display lines should be reported");
		Expect(appendResult.pendingFragmentBytes == 0, "oversized partial display lines should be discarded");
		Expect(!state.HasPendingLines(), "oversized partial display lines should not queue display lines");
	}
}

void RunDisplayStateTests()
{
	TestOutboxReplacementResetsFromNewHead();
	TestOutboxTruncationResetsQueuedLines();
	TestOutboxRemovalResetsState();
	TestBoundedQueueRefillsFromBufferedFragment();
	TestOversizedFragmentIsDiscarded();
}
