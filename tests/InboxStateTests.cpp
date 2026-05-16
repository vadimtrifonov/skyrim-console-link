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

	Inbox::FileIdentity MakeIdentity(const FileIdentityParts parts)
	{
		Inbox::FileIdentity identity{};
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

	void TestNotReadyStateDoesNotReadInboxContents()
	{
		Inbox::State state;
		const auto identity = MakeIdentity({ 1, 10, 100 });

		const auto observe = state.ObserveFile(identity, 8);
		Expect(observe.readMode == Inbox::ReadMode::kNone, "not-ready inbox state should not read content");
		Expect(observe.readOffset == 0, "not-ready inbox state should not establish a cursor");
		Expect(!state.HasPendingLines(), "not-ready inbox state should not queue content");
	}

	void TestReplacementTreatsCurrentContentsAsNewInput()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 2, 20, 200 });
		const auto replacementIdentity = MakeIdentity({ 3, 30, 300 });

		Expect(state.ObserveFile(originalIdentity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before replacement");
		Expect(state.AppendChunk("old1\n").extractedCount == 1, "append after inbox readiness should queue one line");
		Expect(state.PendingLineCount() == 1, "queued line should be visible before replacement");

		const auto replacement = state.ObserveFile(replacementIdentity, 8);
		Expect(replacement.resetReason == Inbox::ResetReason::kReplaced, "replacement should reset state");
		Expect(replacement.droppedPendingLines == 1, "replacement should drop stale queued lines");
		Expect(replacement.readOffset == 0, "replacement should restart from the rewritten file head");
		Expect(replacement.readMode == Inbox::ReadMode::kRewrite, "replacement contents should be treated as new live input");
		Expect(!state.HasPendingLines(), "replacement should empty the pending queue");
	}

	void TestTruncationTreatsCurrentContentsAsNewInput()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 4, 40, 400 });

		Expect(state.ObserveFile(identity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before truncation coverage");
		Expect(state.AppendChunk("one\ntwo\n").extractedCount == 2, "appended lines should queue after inbox readiness");
		Expect(state.PendingLineCount() == 2, "two lines should be pending before truncation");

		const auto truncated = state.ObserveFile(identity, 3);
		Expect(truncated.resetReason == Inbox::ResetReason::kTruncated, "smaller file size should be treated as truncation");
		Expect(truncated.droppedPendingLines == 2, "truncation should drop stale queued lines");
		Expect(truncated.readOffset == 0, "truncation should restart from the rewritten file head");
		Expect(truncated.readMode == Inbox::ReadMode::kRewrite, "truncated file contents should be treated as new live input");
		Expect(!state.HasPendingLines(), "truncation should empty the pending queue");
	}

	void TestMissingFileResetsState()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 5, 50, 500 });

		Expect(state.ObserveFile(identity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before removal coverage");
		Expect(state.AppendChunk("cmd\n").extractedCount == 1, "appended line should queue before removal");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Inbox::ResetReason::kRemoved, "missing file should reset the inbox state");
		Expect(removed.droppedPendingLines == 1, "missing file should clear stale queued lines");
		Expect(!state.HasPendingLines(), "missing file should empty the pending queue");

		const auto removedAgain = state.ObserveMissingFile();
		Expect(removedAgain.resetReason == Inbox::ResetReason::kNone, "repeated missing-file observations should be idempotent");
	}

	void TestMissingAfterReadinessMakesRecreatedFileRewriteInput()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto recreatedIdentity = MakeIdentity({ 20, 200, 2000 });

		const auto missing = state.ObserveMissingFile();
		Expect(missing.resetReason == Inbox::ResetReason::kNone, "missing inbox before first tracked file should not log removal");

		const auto recreated = state.ObserveFile(recreatedIdentity, 9);
		Expect(recreated.readMode == Inbox::ReadMode::kRewrite, "recreated inbox after readiness should use rewrite ingestion");
		Expect(recreated.readOffset == 0, "recreated inbox after readiness should read from the file head");
	}

	void TestRecreatedFileTreatsCurrentContentsAsNewInput()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 10, 100, 1000 });
		const auto recreatedIdentity = MakeIdentity({ 11, 110, 1100 });

		Expect(state.ObserveFile(originalIdentity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before recreation coverage");
		Expect(state.AppendChunk("old\n").extractedCount == 1, "append after inbox readiness should queue one line before recreation");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Inbox::ResetReason::kRemoved, "missing file should reset state before recreation");
		Expect(removed.droppedPendingLines == 1, "missing file should drop pending lines from the removed file");

		const auto recreated = state.ObserveFile(recreatedIdentity, 9);
		Expect(recreated.readOffset == 0, "recreated inbox should restart from the file head");
		Expect(recreated.readMode == Inbox::ReadMode::kRewrite, "recreated inbox contents should be treated as new live input");
	}

	void TestEmptyRewriteWaitsForLaterContents()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 18, 180, 1800 });
		const auto replacementIdentity = MakeIdentity({ 19, 190, 1900 });

		Expect(state.ObserveFile(originalIdentity, 4).readMode == Inbox::ReadMode::kAppendCandidate, "ready inbox should append initial contents from an empty cursor");
		const auto emptyReplacement = state.ObserveFile(replacementIdentity, 0);
		Expect(emptyReplacement.resetReason == Inbox::ResetReason::kReplaced, "empty replacement should enter rewrite mode");
		Expect(emptyReplacement.readMode == Inbox::ReadMode::kNone, "empty replacement should wait for later contents");

		const auto laterContents = state.ObserveFile(replacementIdentity, 6);
		Expect(laterContents.readMode == Inbox::ReadMode::kRewrite, "later contents after empty replacement should still use rewrite ingestion");
	}

	void TestUtf8BomIsStrippedFromQueuedLine()
	{
		Inbox::State state;
		state.ResetForNewSession();

		const auto result = state.AppendChunk(
			"\xEF\xBB\xBF"
			"cdbg.lookup Gold001\n");
		Expect(result.extractedCount == 1, "line with UTF-8 BOM should still queue one command");

		const auto front = state.FrontLine();
		Expect(front.has_value(), "line with UTF-8 BOM should be queued");
		Expect(front->text == "cdbg.lookup Gold001", "UTF-8 BOM should be stripped from queued line text");
	}

	void TestRewriteAcceptsFinalLineWithoutTrailingNewline()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 12, 120, 1200 });
		const auto replacementIdentity = MakeIdentity({ 13, 130, 1300 });

		Expect(state.ObserveFile(originalIdentity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should observe the empty file before replacement");
		Expect(state.AppendChunk("old-fragment").pendingFragmentBytes == 12, "append fragment should be carried before replacement");

		const std::string contents = "first\nsecond";
		const auto replacementObservation = Inbox::FileObservation{
			.identity = replacementIdentity,
			.size = contents.size(),
			.lastWriteTime = 1,
		};
		const auto replacement = state.ObserveFile(replacementObservation);
		Expect(replacement.readMode == Inbox::ReadMode::kRewrite, "replacement should use rewrite ingestion");
		Expect(replacement.readOffset == 0, "replacement should read from the file head");

		const auto accepted = state.AcceptRewriteContents(replacementObservation, contents);
		Expect(accepted.extractedCount == 2, "stable rewrite should accept final line without trailing newline");

		const auto first = state.FrontLine();
		Expect(first.has_value(), "first rewrite line should be queued");
		Expect(first->text == "first", "first rewrite line mismatch");
		Expect(state.PopFrontLine(first->sequence), "first rewrite line should pop");

		const auto second = state.FrontLine();
		Expect(second.has_value(), "final rewrite line should be queued");
		Expect(second->text == "second", "final rewrite line mismatch");
	}

	void TestAppendAfterRewriteStartsAtAcceptedRewriteSize()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 21, 210, 2100 });
		const auto replacementIdentity = MakeIdentity({ 22, 220, 2200 });
		const std::string contents = "first";

		Expect(state.ObserveFile(originalIdentity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should observe the empty file before replacement");
		const auto replacementObservation = Inbox::FileObservation{
			.identity = replacementIdentity,
			.size = contents.size(),
			.lastWriteTime = 1,
		};
		Expect(state.ObserveFile(replacementObservation).readMode == Inbox::ReadMode::kRewrite, "replacement should use rewrite ingestion");
		Expect(state.AcceptRewriteContents(replacementObservation, contents).extractedCount == 1, "stable rewrite should queue one line");

		const auto grownObservation = Inbox::FileObservation{
			.identity = replacementIdentity,
			.size = contents.size() + 6,
			.lastWriteTime = 2,
		};
		const auto append = state.ObserveFile(grownObservation);
		Expect(append.readMode == Inbox::ReadMode::kAppendCandidate, "growth after accepted rewrite should use append ingestion");
		Expect(append.readOffset == contents.size(), "growth after accepted rewrite should read from the accepted rewrite size");
	}

	void TestAcceptedPrefixMismatchStartsRewriteForSameIdentityGrowth()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 23, 230, 2300 });
		const std::string original = "cdbg.lookup-prefix CL_SHORT_3 1\n";
		const std::string overwritten = "cdbg.lookup-prefix CL_LONG_OVERWRITE_20260503_4_EXTRA_LONG_MARKER 1\n";

		Expect(state.ObserveFile(identity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should observe the empty file before append");
		Expect(state.AppendChunk(original).extractedCount == 1, "original append should queue one line");

		const auto mismatch = state.AppendObservedContents(original.size(), overwritten);
		Expect(mismatch.resetReason == Inbox::ResetReason::kOverwritten, "accepted prefix mismatch should report overwrite");
		Expect(mismatch.readOffset == 0, "accepted prefix mismatch should reset the read offset");

		const auto rewrite = state.ObserveFile(identity, overwritten.size());
		Expect(rewrite.readMode == Inbox::ReadMode::kRewrite, "same-identity overwrite growth should be read as rewrite after prefix mismatch");
		Expect(rewrite.readOffset == 0, "same-identity overwrite growth should read from the file head after prefix mismatch");
	}

	void TestSameSizeOverwriteStartsRewriteWhenWriteTimeChanges()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 24, 240, 2400 });
		const auto originalObservation = Inbox::FileObservation{
			.identity = identity,
			.size = 0,
			.lastWriteTime = 1,
		};
		const auto overwrittenObservation = Inbox::FileObservation{
			.identity = identity,
			.size = 4,
			.lastWriteTime = 2,
		};

		Expect(state.ObserveFile(originalObservation).readMode == Inbox::ReadMode::kNone, "ready inbox should observe the empty file before same-size overwrite");
		Expect(state.AppendChunk("old\n").extractedCount == 1, "original append should queue one line before same-size overwrite");

		const auto overwritten = state.ObserveFile(overwrittenObservation);
		Expect(overwritten.resetReason == Inbox::ResetReason::kOverwritten, "same-size overwrite should reset when the write time changes");
		Expect(overwritten.droppedPendingLines == 1, "same-size overwrite should drop pending old lines");
		Expect(overwritten.readMode == Inbox::ReadMode::kRewrite, "same-size overwrite should be read as rewrite");
		Expect(overwritten.readOffset == 0, "same-size overwrite should read from the file head");
	}

	void TestQueueFullStillDetectsSameIdentityOverwriteGrowth()
	{
		Inbox::State state(Inbox::StateLimits{
			.maxPendingLines = 1,
			.maxPendingFragmentBytes = static_cast<std::size_t>(8) * 1024 });
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 25, 250, 2500 });
		const std::string original = "old\n";
		const std::string appended = "old\nnew\n";
		const std::string overwritten = "replacement\n";

		Expect(state.ObserveFile(identity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should observe the empty file before queue-full overwrite coverage");
		Expect(state.AppendChunk(original).queueAtCapacity, "original line should fill the queue");

		const auto growth = state.ObserveFile(identity, appended.size());
		Expect(growth.queueAtCapacity, "growth while full should still report queue capacity");
		Expect(growth.readMode == Inbox::ReadMode::kAppendCandidate, "growth while full should still validate the append candidate");

		const auto matching = state.AppendObservedContents(original.size(), appended);
		Expect(matching.queueAtCapacity, "matching append candidate should remain paused while the queue is full");
		Expect(matching.readOffset == original.size(), "matching append candidate should not advance the cursor while the queue is full");
		Expect(state.PendingLineCount() == 1, "matching append candidate should not ingest more lines while full");

		const auto mismatch = state.AppendObservedContents(original.size(), overwritten);
		Expect(mismatch.resetReason == Inbox::ResetReason::kOverwritten, "overwrite should be detected even while the queue is full");
		Expect(mismatch.droppedPendingLines == 1, "overwrite should drop stale queued lines even while the queue is full");
		Expect(!state.HasPendingLines(), "overwrite should clear the full stale queue");
	}

	void TestAcceptedRewriteIsNotReplayedUntilAnotherRewrite()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 14, 140, 1400 });
		const auto replacementIdentity = MakeIdentity({ 15, 150, 1500 });
		const auto laterReplacementIdentity = MakeIdentity({ 16, 160, 1600 });
		const std::string contents = "same";

		Expect(state.ObserveFile(originalIdentity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should observe the empty file before replacement");

		const auto replacementObservation = Inbox::FileObservation{
			.identity = replacementIdentity,
			.size = contents.size(),
			.lastWriteTime = 1,
		};
		Expect(state.ObserveFile(replacementObservation).readMode == Inbox::ReadMode::kRewrite, "replacement should use rewrite ingestion");
		Expect(state.AcceptRewriteContents(replacementObservation, contents).extractedCount == 1, "stable rewrite should queue one line");

		const auto sameObservation = state.ObserveFile(replacementObservation);
		Expect(sameObservation.readMode == Inbox::ReadMode::kNone, "same accepted rewrite observation should not replay");

		const auto laterReplacementObservation = Inbox::FileObservation{
			.identity = laterReplacementIdentity,
			.size = contents.size(),
			.lastWriteTime = 2,
		};
		const auto laterReplacement = state.ObserveFile(laterReplacementObservation);
		Expect(laterReplacement.readMode == Inbox::ReadMode::kRewrite, "later replacement should start a new rewrite");
	}

	void TestBoundedQueueRefillsFromBufferedFragment()
	{
		Inbox::State state(Inbox::StateLimits{
			.maxPendingLines = 2,
			.maxPendingFragmentBytes = static_cast<std::size_t>(8) * 1024 });
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 6, 60, 600 });

		Expect(state.ObserveFile(identity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before queue-cap coverage");
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
		Expect(refillResult.readMode == Inbox::ReadMode::kNone, "refill from buffered data should not require another file read");

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
		Inbox::State state(Inbox::StateLimits{
			.maxPendingLines = 4,
			.maxPendingFragmentBytes = 4 });
		state.ResetForNewSession();
		const auto identity = MakeIdentity({ 7, 70, 700 });

		Expect(state.ObserveFile(identity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before oversized-fragment coverage");
		const auto appendResult = state.AppendChunk("abcde");
		Expect(appendResult.oversizedFragmentBytes == 5, "oversized partial lines should be reported");
		Expect(appendResult.pendingFragmentBytes == 0, "oversized partial lines should be discarded");
		Expect(!state.HasPendingLines(), "oversized partial lines should not queue input");
	}

	void TestStaleFrontSequenceDoesNotPopReplacementLine()
	{
		Inbox::State state;
		state.ResetForNewSession();
		const auto originalIdentity = MakeIdentity({ 8, 80, 800 });
		const auto replacementIdentity = MakeIdentity({ 9, 90, 900 });

		Expect(state.ObserveFile(originalIdentity, 0).readMode == Inbox::ReadMode::kNone, "ready inbox should track the empty file before stale-sequence coverage");
		Expect(state.AppendChunk("cmd\n").extractedCount == 1, "original line should queue before replacement");
		const auto originalFront = state.FrontLine();
		Expect(originalFront.has_value(), "original front line should exist");
		const auto originalSequence = originalFront->sequence;

		const auto replacement = state.ObserveFile(replacementIdentity, 4);
		Expect(replacement.resetReason == Inbox::ResetReason::kReplaced, "replacement should reset state before stale-sequence coverage");
		Expect(replacement.readMode == Inbox::ReadMode::kRewrite, "replacement contents should be treated as new live input");
		Expect(!state.PopFrontLine(originalSequence), "stale front sequence should not pop a later replacement line");
	}
}

void RunInboxStateTests()
{
	TestWorkerStartState();
	TestNotReadyStateDoesNotReadInboxContents();
	TestReplacementTreatsCurrentContentsAsNewInput();
	TestTruncationTreatsCurrentContentsAsNewInput();
	TestMissingFileResetsState();
	TestMissingAfterReadinessMakesRecreatedFileRewriteInput();
	TestRecreatedFileTreatsCurrentContentsAsNewInput();
	TestEmptyRewriteWaitsForLaterContents();
	TestUtf8BomIsStrippedFromQueuedLine();
	TestRewriteAcceptsFinalLineWithoutTrailingNewline();
	TestAppendAfterRewriteStartsAtAcceptedRewriteSize();
	TestAcceptedPrefixMismatchStartsRewriteForSameIdentityGrowth();
	TestSameSizeOverwriteStartsRewriteWhenWriteTimeChanges();
	TestQueueFullStillDetectsSameIdentityOverwriteGrowth();
	TestAcceptedRewriteIsNotReplayedUntilAnotherRewrite();
	TestBoundedQueueRefillsFromBufferedFragment();
	TestOversizedFragmentIsDiscarded();
	TestStaleFrontSequenceDoesNotPopReplacementLine();
}
