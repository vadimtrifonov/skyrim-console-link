#include "command/CommandSubmissionState.h"

#include <algorithm>

namespace
{
	std::string TrimCommandLine(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}
		return value;
	}
}

namespace Command
{
	InboxState::InboxState(const std::size_t maxPendingCommands, const std::size_t maxPendingFragmentBytes) :
		_maxPendingCommands(maxPendingCommands),
		_maxPendingFragmentBytes(maxPendingFragmentBytes)
	{}

	InboxUpdateResult InboxState::ObserveMissingFile()
	{
		if (!_fileIdentityKnown && _fileCursor == 0 && _pendingFragment.empty() && _pendingCommands.empty()) {
			return {};
		}

		auto result = ResetState(InboxResetReason::kRemoved);
		_fileIdentityKnown = false;
		return result;
	}

	InboxUpdateResult InboxState::ObserveFile(const InboxFileIdentity& identity, const std::uint64_t fileSize)
	{
		InboxUpdateResult result{};

		if (_fileIdentityKnown && !(identity == _fileIdentity)) {
			result = ResetState(InboxResetReason::kReplaced);
		}

		_fileIdentity = identity;
		_fileIdentityKnown = true;

		if (fileSize < _fileCursor) {
			result = ResetState(InboxResetReason::kTruncated);
		}

		ExtractCompleteLines(result);
		result.pendingCount = _pendingCommands.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingCommands.size() >= _maxPendingCommands;
		result.shouldRead = !result.queueAtCapacity && fileSize > _fileCursor;
		return result;
	}

	InboxUpdateResult InboxState::AppendChunk(std::string_view chunk)
	{
		InboxUpdateResult result{};
		_fileCursor += chunk.size();
		_pendingFragment.append(chunk);
		ExtractCompleteLines(result);
		result.pendingCount = _pendingCommands.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingCommands.size() >= _maxPendingCommands;
		return result;
	}

	bool InboxState::HasPendingCommands() const noexcept
	{
		return !_pendingCommands.empty();
	}

	std::size_t InboxState::PendingCommandCount() const noexcept
	{
		return _pendingCommands.size();
	}

	std::optional<PendingCommand> InboxState::FrontCommand() const
	{
		if (_pendingCommands.empty()) {
			return std::nullopt;
		}

		return _pendingCommands.front();
	}

	bool InboxState::PopFrontCommand(const std::uint64_t sequence)
	{
		if (!_pendingCommands.empty() && _pendingCommands.front().sequence == sequence) {
			_pendingCommands.pop_front();
			return true;
		}

		return false;
	}

	std::size_t InboxState::MaxPendingCommands() const noexcept
	{
		return _maxPendingCommands;
	}

	InboxUpdateResult InboxState::ResetState(const InboxResetReason reason)
	{
		InboxUpdateResult result{};
		result.resetReason = reason;
		result.droppedPendingCommands = _pendingCommands.size();
		_fileCursor = 0;
		_pendingFragment.clear();
		_pendingCommands.clear();
		result.pendingCount = 0;
		result.pendingFragmentBytes = 0;
		result.readOffset = 0;
		return result;
	}

	void InboxState::ExtractCompleteLines(InboxUpdateResult& result)
	{
		while (_pendingCommands.size() < _maxPendingCommands) {
			const auto newline = _pendingFragment.find('\n');
			if (newline == std::string::npos) {
				break;
			}

			auto line = TrimCommandLine(_pendingFragment.substr(0, newline));
			_pendingFragment.erase(0, newline + 1);

			if (!line.empty()) {
				_pendingCommands.push_back(PendingCommand{ _nextCommandSequence++, std::move(line) });
				++result.extractedCount;
			}
		}

		if (_pendingCommands.size() >= _maxPendingCommands) {
			result.queueAtCapacity = true;
			result.pendingCount = _pendingCommands.size();
			result.pendingFragmentBytes = _pendingFragment.size();
			return;
		}

		if (_pendingFragment.size() > _maxPendingFragmentBytes) {
			result.oversizedFragmentBytes = _pendingFragment.size();
			_pendingFragment.clear();
		}

		result.pendingCount = _pendingCommands.size();
		result.pendingFragmentBytes = _pendingFragment.size();
	}
}
