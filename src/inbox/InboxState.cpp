#include "inbox/InboxState.h"

namespace
{
	constexpr std::string_view kUtf8Bom{ "\xEF\xBB\xBF", 3 };

	std::string TrimLine(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}

		if (value.starts_with(kUtf8Bom)) {
			value.erase(0, kUtf8Bom.size());
		}

		return value;
	}
}

namespace Inbox
{
	State::State(const StateLimits limits) :
		_maxPendingLines(limits.maxPendingLines),
		_maxPendingFragmentBytes(limits.maxPendingFragmentBytes)
	{}

	void State::ResetForNewSession() noexcept
	{
		_pendingLines.clear();
		_pendingFragment.clear();
		_acceptedContents.clear();
		_fileCursor = 0;
		_fileLastWriteTime = 0;
		_trackingState = TrackingState::kReadyNoTrackedFile;
	}

	UpdateResult State::ObserveMissingFile()
	{
		switch (_trackingState) {
		case TrackingState::kReadyNoTrackedFile:
			return EnterRewrite(ResetReason::kNone);
		case TrackingState::kTrackingAppend:
			return EnterRewrite(ResetReason::kRemoved);
		case TrackingState::kNotReady:
		case TrackingState::kRewritePending:
			return {};
		}

		return {};
	}

	UpdateResult State::ObserveFile(const FileObservation& observation)
	{
		UpdateResult result{};

		switch (_trackingState) {
		case TrackingState::kNotReady:
			return result;

		case TrackingState::kReadyNoTrackedFile:
			_fileIdentity = observation.identity;
			_fileLastWriteTime = observation.lastWriteTime;
			_trackingState = TrackingState::kTrackingAppend;
			break;

		case TrackingState::kRewritePending:
			_fileIdentity = observation.identity;
			result.readOffset = 0;
			result.readMode = observation.size > 0 ? ReadMode::kRewrite : ReadMode::kNone;
			return result;

		case TrackingState::kTrackingAppend:
			if (!(observation.identity == _fileIdentity)) {
				result = EnterRewrite(ResetReason::kReplaced);
			} else if (observation.size < _fileCursor) {
				result = EnterRewrite(ResetReason::kTruncated);
			} else if (observation.size == _fileCursor && observation.lastWriteTime != _fileLastWriteTime) {
				result = EnterRewrite(ResetReason::kOverwritten);
			}

			_fileIdentity = observation.identity;
			if (_trackingState == TrackingState::kRewritePending) {
				result.readOffset = 0;
				result.readMode = observation.size > 0 ? ReadMode::kRewrite : ReadMode::kNone;
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
		result.readMode = observation.size > _fileCursor ? ReadMode::kAppendCandidate : ReadMode::kNone;
		return result;
	}

	UpdateResult State::ObserveFile(const FileIdentity& identity, const std::uint64_t fileSize)
	{
		return ObserveFile(FileObservation{
			.identity = identity,
			.size = fileSize,
		});
	}

	UpdateResult State::AppendObservedContents(const std::uint64_t appendOffset, const std::string_view observedContents)
	{
		if (appendOffset > observedContents.size() || appendOffset != _fileCursor) {
			return EnterRewrite(ResetReason::kOverwritten);
		}

		const auto prefixSize = static_cast<std::size_t>(appendOffset);
		if (std::string_view(observedContents.data(), prefixSize) != _acceptedContents) {
			return EnterRewrite(ResetReason::kOverwritten);
		}

		if (_pendingLines.size() >= _maxPendingLines) {
			UpdateResult result{};
			result.pendingCount = _pendingLines.size();
			result.pendingFragmentBytes = _pendingFragment.size();
			result.readOffset = _fileCursor;
			result.queueAtCapacity = true;
			return result;
		}

		return AppendChunk(observedContents.substr(prefixSize));
	}

	UpdateResult State::AppendChunk(std::string_view chunk)
	{
		UpdateResult result{};
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

	UpdateResult State::AcceptRewriteContents(const FileObservation& stableObservation, std::string_view contents)
	{
		UpdateResult result{};
		_fileIdentity = stableObservation.identity;
		_fileCursor = stableObservation.size;
		_fileLastWriteTime = stableObservation.lastWriteTime;
		_acceptedContents.assign(contents);
		_trackingState = TrackingState::kTrackingAppend;

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

	bool State::HasPendingLines() const noexcept
	{
		return !_pendingLines.empty();
	}

	std::size_t State::PendingLineCount() const noexcept
	{
		return _pendingLines.size();
	}

	std::optional<PendingLine> State::FrontLine() const
	{
		if (_pendingLines.empty()) {
			return std::nullopt;
		}

		return _pendingLines.front();
	}

	bool State::PopFrontLine(const std::uint64_t sequence)
	{
		if (!_pendingLines.empty() && _pendingLines.front().sequence == sequence) {
			_pendingLines.pop_front();
			return true;
		}

		return false;
	}

	std::size_t State::MaxPendingLines() const noexcept
	{
		return _maxPendingLines;
	}

	UpdateResult State::ResetState(const ResetReason reason, const std::uint64_t baselineOffset)
	{
		UpdateResult result{};
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

	UpdateResult State::EnterRewrite(const ResetReason reason)
	{
		auto result = ResetState(reason, 0);
		_trackingState = TrackingState::kRewritePending;
		return result;
	}

	void State::ExtractCompleteLines(UpdateResult& result)
	{
		while (_pendingLines.size() < _maxPendingLines) {
			const auto newline = _pendingFragment.find('\n');
			if (newline == std::string::npos) {
				break;
			}

			auto line = TrimLine(_pendingFragment.substr(0, newline));
			_pendingFragment.erase(0, newline + 1);
			if (!line.empty()) {
				_pendingLines.push_back(PendingLine{ _nextSequence++, std::move(line) });
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
