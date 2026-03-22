#include "command/CommandSubmissionState.h"

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

	Command::InboxFileIdentity MakeIdentity(
		const std::uint32_t volumeSerialNumber,
		const std::uint32_t fileIndexHigh,
		const std::uint32_t fileIndexLow)
	{
		Command::InboxFileIdentity identity{};
		identity.volumeSerialNumber = volumeSerialNumber;
		identity.fileIndexHigh = fileIndexHigh;
		identity.fileIndexLow = fileIndexLow;
		return identity;
	}

	void TestWorkerStartState()
	{
		Command::WorkerStartState state;

		Expect(state.CanStart(false), "worker should be startable before startup");
		Expect(!state.CanStart(true), "worker should not start when the service is disabled");
		Expect(!state.Started(), "worker should not be marked started before success");

		state.MarkStarted();

		Expect(state.Started(), "worker should publish a started state after success");
		Expect(!state.CanStart(false), "worker should not start twice");
	}

	void TestInboxReplacementResetsFromNewHead()
	{
		Command::InboxState state;
		const auto         originalIdentity = MakeIdentity(1, 10, 100);
		const auto         replacementIdentity = MakeIdentity(2, 20, 200);

		const auto observeOriginal = state.ObserveFile(originalIdentity, 8);
		Expect(observeOriginal.shouldRead, "fresh inbox should read from offset zero");
		Expect(observeOriginal.readOffset == 0, "fresh inbox should start at the file head");

		const auto appendOriginal = state.AppendChunk("old1\n");
		Expect(appendOriginal.extractedCount == 1, "original inbox content should queue one command");
		Expect(state.PendingCommandCount() == 1, "original inbox command should remain pending");

		const auto observeReplacement = state.ObserveFile(replacementIdentity, 8);
		Expect(observeReplacement.resetReason == Command::InboxResetReason::kReplaced, "replacement should reset the inbox state");
		Expect(observeReplacement.droppedPendingCommands == 1, "replacement should drop stale queued commands");
		Expect(observeReplacement.readOffset == 0, "replacement should reread from the new file head");
		Expect(observeReplacement.shouldRead, "replacement should request a read from the new file head");
		Expect(state.PendingCommandCount() == 0, "replacement should clear the pending queue");
	}

	void TestInboxTruncationResetsQueuedCommands()
	{
		Command::InboxState state;
		const auto         identity = MakeIdentity(3, 30, 300);

		const auto observeOriginal = state.ObserveFile(identity, 8);
		Expect(observeOriginal.shouldRead, "fresh inbox should read before truncation coverage");
		const auto appendOriginal = state.AppendChunk("one\ntwo\n");
		Expect(appendOriginal.extractedCount == 2, "original inbox content should queue two commands");
		Expect(state.PendingCommandCount() == 2, "two commands should be pending before truncation");

		const auto observeTruncated = state.ObserveFile(identity, 3);
		Expect(observeTruncated.resetReason == Command::InboxResetReason::kTruncated, "smaller file size should be treated as truncation");
		Expect(observeTruncated.droppedPendingCommands == 2, "truncation should clear stale queued commands");
		Expect(observeTruncated.readOffset == 0, "truncation should restart reading from the file head");
		Expect(observeTruncated.shouldRead, "truncation should request a reread from the new file head");
		Expect(state.PendingCommandCount() == 0, "truncation should empty the pending queue");
	}

	void TestInboxRemovalResetsState()
	{
		Command::InboxState state;
		const auto         identity = MakeIdentity(4, 40, 400);

		const auto observeOriginal = state.ObserveFile(identity, 4);
		Expect(observeOriginal.shouldRead, "fresh inbox should read before removal coverage");
		const auto appendOriginal = state.AppendChunk("cmd\n");
		Expect(appendOriginal.extractedCount == 1, "original inbox content should queue one command");

		const auto removed = state.ObserveMissingFile();
		Expect(removed.resetReason == Command::InboxResetReason::kRemoved, "missing file should reset the inbox state");
		Expect(removed.droppedPendingCommands == 1, "missing file should clear stale queued commands");
		Expect(!state.HasPendingCommands(), "missing file should empty the pending queue");

		const auto removedAgain = state.ObserveMissingFile();
		Expect(removedAgain.resetReason == Command::InboxResetReason::kNone, "repeated missing-file observations should be idempotent");
	}

	void TestBoundedQueueRefillsFromBufferedFragment()
	{
		Command::InboxState state(2, 8 * 1024);
		const auto         identity = MakeIdentity(5, 50, 500);

		const auto observeOriginal = state.ObserveFile(identity, 6);
		Expect(observeOriginal.shouldRead, "fresh inbox should read before queue-cap coverage");
		const auto appendResult = state.AppendChunk("a\nb\nc\n");
		Expect(appendResult.extractedCount == 2, "queue should stop extracting at the configured cap");
		Expect(appendResult.pendingCount == 2, "queue should report the configured cap");
		Expect(appendResult.queueAtCapacity, "queue should mark itself at capacity");
		const auto front = state.FrontCommand();
		Expect(front.has_value(), "first queued command should be visible at the head");
		Expect(front->text == "a", "first queued command should remain at the head");

		Expect(state.PopFrontCommand(front->sequence), "matching front sequence should pop the queued command");

		const auto refillResult = state.ObserveFile(identity, 6);
		Expect(refillResult.extractedCount == 1, "buffered lines should refill the queue once capacity is available");
		Expect(refillResult.pendingCount == 2, "queue should refill back to the configured cap");
		Expect(refillResult.queueAtCapacity, "queue should return to capacity after the refill");
		Expect(!refillResult.shouldRead, "refill from buffered data should not require another file read");
		const auto second = state.FrontCommand();
		Expect(second.has_value(), "second command should be visible after one pop");
		Expect(second->text == "b", "second command should move to the head after one pop");

		Expect(state.PopFrontCommand(second->sequence), "second front sequence should pop cleanly");
		const auto third = state.FrontCommand();
		Expect(third.has_value(), "buffered command should become visible after the refill");
		Expect(third->text == "c", "buffered command should become visible after the refill");
	}

	void TestOversizedFragmentIsDiscarded()
	{
		Command::InboxState state(4, 4);
		const auto         identity = MakeIdentity(6, 60, 600);

		const auto observeOriginal = state.ObserveFile(identity, 5);
		Expect(observeOriginal.shouldRead, "fresh inbox should read before oversized-fragment coverage");
		const auto appendResult = state.AppendChunk("abcde");
		Expect(appendResult.oversizedFragmentBytes == 5, "oversized partial lines should be reported");
		Expect(appendResult.pendingFragmentBytes == 0, "oversized partial lines should be discarded");
		Expect(!state.HasPendingCommands(), "oversized partial lines should not queue commands");
	}

	void TestStaleFrontSequenceDoesNotPopReplacementCommand()
	{
		Command::InboxState state;
		const auto         originalIdentity = MakeIdentity(7, 70, 700);
		const auto         replacementIdentity = MakeIdentity(8, 80, 800);

		Expect(state.ObserveFile(originalIdentity, 4).shouldRead, "fresh inbox should read before stale-sequence coverage");
		Expect(state.AppendChunk("cmd\n").extractedCount == 1, "original command should queue before replacement");
		const auto originalFront = state.FrontCommand();
		Expect(originalFront.has_value(), "original front command should exist");

		const auto replacement = state.ObserveFile(replacementIdentity, 4);
		Expect(replacement.resetReason == Command::InboxResetReason::kReplaced, "replacement should reset state before stale-sequence coverage");
		Expect(state.AppendChunk("cmd\n").extractedCount == 1, "replacement command should queue after reset");
		Expect(!state.PopFrontCommand(originalFront->sequence), "stale front sequence should not pop the replacement command");

		const auto replacementFront = state.FrontCommand();
		Expect(replacementFront.has_value(), "replacement front command should remain queued");
		Expect(replacementFront->text == "cmd", "replacement command text should remain intact");
	}
}

void RunCommandSubmissionStateTests()
{
	TestWorkerStartState();
	TestInboxReplacementResetsFromNewHead();
	TestInboxTruncationResetsQueuedCommands();
	TestInboxRemovalResetsState();
	TestBoundedQueueRefillsFromBufferedFragment();
	TestOversizedFragmentIsDiscarded();
	TestStaleFrontSequenceDoesNotPopReplacementCommand();
}
