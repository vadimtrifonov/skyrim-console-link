#include "print/PrintState.h"

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

	Print::OutboxFileIdentity MakeIdentity(const FileIdentityParts parts)
	{
		Print::OutboxFileIdentity identity{};
		identity.volumeSerialNumber = parts.volumeSerialNumber;
		identity.fileIndexHigh = parts.fileIndexHigh;
		identity.fileIndexLow = parts.fileIndexLow;
		return identity;
	}

	void TestOutboxReplacementResetsFromNewHead()
	{
		Print::OutboxState state;
		const auto originalIdentity = MakeIdentity({ 1, 10, 100 });
		const auto replacementIdentity = MakeIdentity({ 2, 20, 200 });

		const auto observeOriginal = state.ObserveFile(originalIdentity, 9);
		Expect(observeOriginal.shouldRead, "fresh outbox should read from offset zero");
		Expect(observeOriginal.readOffset == 0, "fresh outbox should start at the file head");

		const auto appendOriginal = state.AppendChunk("line1\n");
		Expect(appendOriginal.extractedCount == 1, "original outbox content should queue one print line");
		Expect(state.PendingLineCount() == 1, "original outbox line should remain pending");

		const auto observeReplacement = state.ObserveFile(replacementIdentity, 9);
		Expect(observeReplacement.resetReason == Print::OutboxResetReason::kReplaced, "replacement should reset the outbox state");
		Expect(observeReplacement.droppedPendingLines == 1, "replacement should drop stale queued print lines");
		Expect(observeReplacement.readOffset == 0, "replacement should reread from the new file head");
		Expect(observeReplacement.shouldRead, "replacement should request a read from the new file head");
		Expect(state.PendingLineCount() == 0, "replacement should clear the pending print queue");
	}

	void TestOutboxTruncationResetsQueuedLines()
	{
		Print::OutboxState state;
		const auto identity = MakeIdentity({ 3, 30, 300 });

		const auto observeOriginal = state.ObserveFile(identity, 10);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before truncation coverage");
		const auto appendOriginal = state.AppendChunk("one\ntwo\n");
		Expect(appendOriginal.extractedCount == 2, "original outbox content should queue two print lines");
		Expect(state.PendingLineCount() == 2, "two print lines should be pending before truncation");

		const auto observeTruncated = state.ObserveFile(identity, 3);
		Expect(observeTruncated.resetReason == Print::OutboxResetReason::kTruncated, "smaller file size should be treated as truncation");
		Expect(observeTruncated.droppedPendingLines == 2, "truncation should clear stale queued print lines");
		Expect(observeTruncated.readOffset == 0, "truncation should restart reading from the file head");
		Expect(observeTruncated.shouldRead, "truncation should request a reread from the new file head");
		Expect(state.PendingLineCount() == 0, "truncation should empty the pending print queue");
	}

	void TestOutboxRemovalResetsState()
	{
		Print::OutboxState state;
		const auto identity = MakeIdentity({ 4, 40, 400 });

		const auto observeOriginal = state.ObserveFile(identity, 5);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before removal coverage");
		const auto appendOriginal = state.AppendChunk("msg\n");
		Expect(appendOriginal.extractedCount == 1, "original outbox content should queue one print line");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Print::OutboxResetReason::kRemoved, "missing file should reset the outbox state");
		Expect(removed.droppedPendingLines == 1, "missing file should clear stale queued print lines");
		Expect(!state.HasPendingLines(), "missing file should empty the pending print queue");

		const auto removedAgain = state.ObserveMissingFile();
		Expect(removedAgain.resetReason == Print::OutboxResetReason::kNone, "repeated missing-file observations should be idempotent");
	}

	void TestBoundedQueueRefillsFromBufferedFragment()
	{
		Print::OutboxState state(Print::OutboxStateLimits{
			.maxPendingLines = 2,
			.maxPendingFragmentBytes = static_cast<std::size_t>(8) * 1024 });
		const auto identity = MakeIdentity({ 5, 50, 500 });

		const auto observeOriginal = state.ObserveFile(identity, 6);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before queue-cap coverage");
		const auto appendResult = state.AppendChunk("a\nb\nc\n");
		Expect(appendResult.extractedCount == 2, "queue should stop extracting at the configured cap");
		Expect(appendResult.pendingCount == 2, "queue should report the configured cap");
		Expect(appendResult.queueAtCapacity, "queue should mark itself at capacity");

		const auto drained = state.DrainLines(1);
		Expect(drained.size() == 1, "draining one print line should return exactly one line");
		Expect(drained.front() == "a", "the first drained print line should be the queue head");

		const auto refillResult = state.ObserveFile(identity, 6);
		Expect(refillResult.extractedCount == 1, "buffered lines should refill the print queue once capacity is available");
		Expect(refillResult.pendingCount == 2, "print queue should refill back to the configured cap");
		Expect(refillResult.queueAtCapacity, "print queue should return to capacity after the refill");
		Expect(!refillResult.shouldRead, "refill from buffered data should not require another file read");

		const auto drainedRemaining = state.DrainLines(2);
		Expect(drainedRemaining.size() == 2, "the remaining buffered print lines should now be drainable");
		Expect(drainedRemaining[0] == "b", "second print line should move to the head after one drain");
		Expect(drainedRemaining[1] == "c", "buffered print line should become visible after the refill");
	}

	void TestOversizedFragmentIsDiscarded()
	{
		Print::OutboxState state(Print::OutboxStateLimits{
			.maxPendingLines = 4,
			.maxPendingFragmentBytes = 4 });
		const auto identity = MakeIdentity({ 6, 60, 600 });

		const auto observeOriginal = state.ObserveFile(identity, 5);
		Expect(observeOriginal.shouldRead, "fresh outbox should read before oversized-fragment coverage");
		const auto appendResult = state.AppendChunk("abcde");
		Expect(appendResult.oversizedFragmentBytes == 5, "oversized partial print lines should be reported");
		Expect(appendResult.pendingFragmentBytes == 0, "oversized partial print lines should be discarded");
		Expect(!state.HasPendingLines(), "oversized partial print lines should not queue print lines");
	}
}

void RunPrintStateTests()
{
	TestOutboxReplacementResetsFromNewHead();
	TestOutboxTruncationResetsQueuedLines();
	TestOutboxRemovalResetsState();
	TestBoundedQueueRefillsFromBufferedFragment();
	TestOversizedFragmentIsDiscarded();
}
