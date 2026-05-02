#include "inbox/InboxState.h"

namespace
{
	std::string TrimLine(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}

		return value;
	}
}

namespace Inbox
{
	InboxState::InboxState(const InboxStateLimits limits) :
		_maxPendingLines(limits.maxPendingLines),
		_maxPendingFragmentBytes(limits.maxPendingFragmentBytes)
	{}

	void InboxState::ResetForNewSession() noexcept
	{
		_pendingLines.clear();
		_pendingFragment.clear();
		_acceptedContents.clear();
		_fileCursor = 0;
		_fileLastWriteTime = 0;
		_trackingState = InboxTrackingState::kReadyNoTrackedFile;
	}

	InboxUpdateResult InboxState::ObserveMissingFile()
	{
		switch (_trackingState) {
		case InboxTrackingState::kReadyNoTrackedFile:
			return EnterRewrite(InboxResetReason::kNone);
		case InboxTrackingState::kTrackingAppend:
			return EnterRewrite(InboxResetReason::kRemoved);
		case InboxTrackingState::kNotReady:
		case InboxTrackingState::kRewritePending:
			return {};
		}

		return {};
	}

	InboxUpdateResult InboxState::ObserveFile(const FileObservation& observation)
	{
		InboxUpdateResult result{};

		switch (_trackingState) {
		case InboxTrackingState::kNotReady:
			return result;

		case InboxTrackingState::kReadyNoTrackedFile:
			_fileIdentity = observation.identity;
			_fileLastWriteTime = observation.lastWriteTime;
			_trackingState = InboxTrackingState::kTrackingAppend;
			break;

		case InboxTrackingState::kRewritePending:
			_fileIdentity = observation.identity;
			result.readOffset = 0;
			result.readMode = observation.size > 0 ? InboxReadMode::kRewrite : InboxReadMode::kNone;
			return result;

		case InboxTrackingState::kTrackingAppend:
			if (!(observation.identity == _fileIdentity)) {
				result = EnterRewrite(InboxResetReason::kReplaced);
			} else if (observation.size < _fileCursor) {
				result = EnterRewrite(InboxResetReason::kTruncated);
			} else if (observation.size == _fileCursor && observation.lastWriteTime != _fileLastWriteTime) {
				result = EnterRewrite(InboxResetReason::kOverwritten);
			}

			_fileIdentity = observation.identity;
			if (_trackingState == InboxTrackingState::kRewritePending) {
				result.readOffset = 0;
				result.readMode = observation.size > 0 ? InboxReadMode::kRewrite : InboxReadMode::kNone;
				return result;
			}
			_fileLastWriteTime = observation.lastWriteTime;
			break;
		}

		ExtractCompleteLines(result);
		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingLines.size() >= _maxPendingLines;
		result.readMode = observation.size > _fileCursor ? InboxReadMode::kAppendCandidate : InboxReadMode::kNone;
		return result;
	}

	InboxUpdateResult InboxState::ObserveFile(const InboxFileIdentity& identity, const std::uint64_t fileSize)
	{
		return ObserveFile(FileObservation{
			.identity = identity,
			.size = fileSize,
		});
	}

	InboxUpdateResult InboxState::AppendObservedContents(const std::uint64_t appendOffset, const std::string_view observedContents)
	{
		if (appendOffset > observedContents.size() || appendOffset != _fileCursor) {
			return EnterRewrite(InboxResetReason::kOverwritten);
		}

		const auto prefixSize = static_cast<std::size_t>(appendOffset);
		if (std::string_view(observedContents.data(), prefixSize) != _acceptedContents) {
			return EnterRewrite(InboxResetReason::kOverwritten);
		}

		if (_pendingLines.size() >= _maxPendingLines) {
			InboxUpdateResult result{};
			result.pendingCount = _pendingLines.size();
			result.pendingFragmentBytes = _pendingFragment.size();
			result.readOffset = _fileCursor;
			result.queueAtCapacity = true;
			return result;
		}

		return AppendChunk(observedContents.substr(prefixSize));
	}

	InboxUpdateResult InboxState::AppendChunk(std::string_view chunk)
	{
		InboxUpdateResult result{};
		_fileCursor += chunk.size();
		_acceptedContents.append(chunk);
		_pendingFragment.append(chunk);
		ExtractCompleteLines(result);
		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingLines.size() >= _maxPendingLines;
		return result;
	}

	InboxUpdateResult InboxState::AcceptRewriteContents(const FileObservation& stableObservation, std::string_view contents)
	{
		InboxUpdateResult result{};
		_fileIdentity = stableObservation.identity;
		_fileCursor = stableObservation.size;
		_fileLastWriteTime = stableObservation.lastWriteTime;
		_acceptedContents.assign(contents);
		_trackingState = InboxTrackingState::kTrackingAppend;

		_pendingFragment.assign(contents);
		if (!_pendingFragment.empty() && _pendingFragment.back() != '\n') {
			_pendingFragment.push_back('\n');
		}

		ExtractCompleteLines(result);
		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingLines.size() >= _maxPendingLines;
		return result;
	}

	bool InboxState::HasPendingLines() const noexcept
	{
		return !_pendingLines.empty();
	}

	std::size_t InboxState::PendingLineCount() const noexcept
	{
		return _pendingLines.size();
	}

	std::optional<PendingInputLine> InboxState::FrontLine() const
	{
		if (_pendingLines.empty()) {
			return std::nullopt;
		}

		return _pendingLines.front();
	}

	bool InboxState::PopFrontLine(const std::uint64_t sequence)
	{
		if (!_pendingLines.empty() && _pendingLines.front().sequence == sequence) {
			_pendingLines.pop_front();
			return true;
		}

		return false;
	}

	std::size_t InboxState::MaxPendingLines() const noexcept
	{
		return _maxPendingLines;
	}

	InboxUpdateResult InboxState::ResetState(const InboxResetReason reason, const std::uint64_t baselineOffset)
	{
		InboxUpdateResult result{};
		result.resetReason = reason;
		result.droppedPendingLines = _pendingLines.size();
		_pendingLines.clear();
		_pendingFragment.clear();
		_acceptedContents.clear();
		_fileCursor = baselineOffset;
		_fileLastWriteTime = 0;
		result.readOffset = _fileCursor;
		result.pendingCount = 0;
		result.pendingFragmentBytes = 0;
		return result;
	}

	InboxUpdateResult InboxState::EnterRewrite(const InboxResetReason reason)
	{
		auto result = ResetState(reason, 0);
		_trackingState = InboxTrackingState::kRewritePending;
		return result;
	}

	void InboxState::ExtractCompleteLines(InboxUpdateResult& result)
	{
		while (_pendingLines.size() < _maxPendingLines) {
			const auto newline = _pendingFragment.find('\n');
			if (newline == std::string::npos) {
				break;
			}

			auto line = TrimLine(_pendingFragment.substr(0, newline));
			_pendingFragment.erase(0, newline + 1);
			if (!line.empty()) {
				_pendingLines.push_back(PendingInputLine{ _nextSequence++, std::move(line) });
				++result.extractedCount;
			}
		}

		if (_pendingLines.size() >= _maxPendingLines) {
			result.queueAtCapacity = true;
			result.pendingCount = _pendingLines.size();
			result.pendingFragmentBytes = _pendingFragment.size();
			return;
		}

		if (_pendingFragment.size() > _maxPendingFragmentBytes) {
			result.oversizedFragmentBytes = _pendingFragment.size();
			_pendingFragment.clear();
		}

		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
	}
}
