#include "print/PrintState.h"

#include <algorithm>

namespace
{
	std::string TrimPrintLine(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}
		return value;
	}
}

namespace Print
{
	OutboxState::OutboxState(const OutboxStateLimits limits) :
		_maxPendingLines(limits.maxPendingLines),
		_maxPendingFragmentBytes(limits.maxPendingFragmentBytes)
	{}

	OutboxUpdateResult OutboxState::ObserveMissingFile()
	{
		if (!_fileIdentityKnown && _fileCursor == 0 && _pendingFragment.empty() && _pendingLines.empty()) {
			return {};
		}

		auto result = ResetState(OutboxResetReason::kRemoved);
		_fileIdentityKnown = false;
		return result;
	}

	OutboxUpdateResult OutboxState::ObserveFile(const OutboxFileIdentity& identity, const std::uint64_t fileSize)
	{
		OutboxUpdateResult result{};

		if (_fileIdentityKnown && !(identity == _fileIdentity)) {
			result = ResetState(OutboxResetReason::kReplaced);
		}

		_fileIdentity = identity;
		_fileIdentityKnown = true;

		if (fileSize < _fileCursor) {
			result = ResetState(OutboxResetReason::kTruncated);
		}

		ExtractCompleteLines(result);
		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingLines.size() >= _maxPendingLines;
		result.shouldRead = !result.queueAtCapacity && fileSize > _fileCursor;
		return result;
	}

	OutboxUpdateResult OutboxState::AppendChunk(std::string_view chunk)
	{
		OutboxUpdateResult result{};
		_fileCursor += chunk.size();
		_pendingFragment.append(chunk);
		ExtractCompleteLines(result);
		result.pendingCount = _pendingLines.size();
		result.pendingFragmentBytes = _pendingFragment.size();
		result.readOffset = _fileCursor;
		result.queueAtCapacity = _pendingLines.size() >= _maxPendingLines;
		return result;
	}

	bool OutboxState::HasPendingLines() const noexcept
	{
		return !_pendingLines.empty();
	}

	std::size_t OutboxState::PendingLineCount() const noexcept
	{
		return _pendingLines.size();
	}

	std::size_t OutboxState::MaxPendingLines() const noexcept
	{
		return _maxPendingLines;
	}

	std::vector<std::string> OutboxState::DrainLines(const std::size_t maxLines)
	{
		const auto               lineCount = std::min(_pendingLines.size(), maxLines);
		std::vector<std::string> lines;
		lines.reserve(lineCount);
		for (std::size_t index = 0; index < lineCount; ++index) {
			lines.push_back(std::move(_pendingLines.front()));
			_pendingLines.pop_front();
		}
		return lines;
	}

	OutboxUpdateResult OutboxState::ResetState(const OutboxResetReason reason)
	{
		OutboxUpdateResult result{};
		result.resetReason = reason;
		result.droppedPendingLines = _pendingLines.size();
		_fileCursor = 0;
		_pendingFragment.clear();
		_pendingLines.clear();
		result.pendingCount = 0;
		result.pendingFragmentBytes = 0;
		result.readOffset = 0;
		return result;
	}

	void OutboxState::ExtractCompleteLines(OutboxUpdateResult& result)
	{
		while (_pendingLines.size() < _maxPendingLines) {
			const auto newline = _pendingFragment.find('\n');
			if (newline == std::string::npos) {
				break;
			}

			auto line = TrimPrintLine(_pendingFragment.substr(0, newline));
			_pendingFragment.erase(0, newline + 1);

			if (!line.empty()) {
				_pendingLines.push_back(std::move(line));
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
