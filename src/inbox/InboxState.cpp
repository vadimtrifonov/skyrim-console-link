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
		_fileCursor = 0;
		_fileIdentityKnown = false;
		_baselineEstablished = true;
	}

	InboxUpdateResult InboxState::ObserveMissingFile()
	{
		if (!_fileIdentityKnown && _fileCursor == 0 && _pendingFragment.empty() && _pendingLines.empty()) {
			return {};
		}

		auto result = ResetState(InboxResetReason::kRemoved, 0);
		_fileIdentityKnown = false;
		return result;
	}

	InboxUpdateResult InboxState::ObserveFile(const InboxFileIdentity& identity, const std::uint64_t fileSize)
	{
		InboxUpdateResult result{};

		if (!_baselineEstablished) {
			_baselineEstablished = true;
			_fileIdentity = identity;
			_fileIdentityKnown = true;
			_fileCursor = fileSize;
			result.readOffset = _fileCursor;
			return result;
		}

		if (_fileIdentityKnown && !(identity == _fileIdentity)) {
			result = ResetState(InboxResetReason::kReplaced, 0);
		} else if (_fileIdentityKnown && fileSize < _fileCursor) {
			result = ResetState(InboxResetReason::kTruncated, 0);
		}

		_fileIdentity = identity;
		_fileIdentityKnown = true;

		ExtractCompleteLines(result);
		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingLines.size() >= _maxPendingLines;
		result.shouldRead = !result.queueAtCapacity && fileSize > _fileCursor;
		return result;
	}

	InboxUpdateResult InboxState::AppendChunk(std::string_view chunk)
	{
		InboxUpdateResult result{};
		_fileCursor += chunk.size();
		_pendingFragment.append(chunk);
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
		_fileCursor = baselineOffset;
		result.readOffset = _fileCursor;
		result.pendingCount = 0;
		result.pendingFragmentBytes = 0;
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
