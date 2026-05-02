#include "inbox/InboxFileReader.h"

#include <Windows.h>

#include <utility>

namespace
{
	[[nodiscard]] std::uint64_t CombineHighLow(const DWORD high, const DWORD low) noexcept
	{
		return (static_cast<std::uint64_t>(high) << 32) | static_cast<std::uint64_t>(low);
	}

	bool TryObserveOpenFile(HANDLE handle, Inbox::FileObservation& observation, DWORD& errorCode) noexcept
	{
		BY_HANDLE_FILE_INFORMATION info{};
		if (!::GetFileInformationByHandle(handle, &info)) {
			errorCode = ::GetLastError();
			return false;
		}

		observation.identity.volumeSerialNumber = info.dwVolumeSerialNumber;
		observation.identity.fileIndexHigh = info.nFileIndexHigh;
		observation.identity.fileIndexLow = info.nFileIndexLow;
		observation.size = CombineHighLow(info.nFileSizeHigh, info.nFileSizeLow);
		observation.lastWriteTime = CombineHighLow(info.ftLastWriteTime.dwHighDateTime, info.ftLastWriteTime.dwLowDateTime);
		errorCode = ERROR_SUCCESS;
		return true;
	}
}

namespace Inbox
{
	InboxFile::InboxFile(void* handle, FileObservation observation) noexcept :
		_handle(handle),
		_observation(observation)
	{}

	InboxFile::~InboxFile() noexcept
	{
		Close();
	}

	InboxFile::InboxFile(InboxFile&& other) noexcept :
		_handle(std::exchange(other._handle, nullptr)),
		_observation(other._observation)
	{}

	InboxFile& InboxFile::operator=(InboxFile&& other) noexcept
	{
		if (this != &other) {
			Close();
			_handle = std::exchange(other._handle, nullptr);
			_observation = other._observation;
		}

		return *this;
	}

	const FileObservation& InboxFile::Observation() const noexcept
	{
		return _observation;
	}

	InboxFileReadResult InboxFile::ReadAt(const std::uint64_t offset, const std::uint64_t byteCount) const noexcept
	{
		if (byteCount > 0xFFFFFFFFull) {
			return InboxFileError{ .operation = "read-size", .code = ERROR_INVALID_PARAMETER };
		}

		LARGE_INTEGER seekOffset{};
		seekOffset.QuadPart = static_cast<LONGLONG>(offset);
		const auto handle = static_cast<HANDLE>(_handle);
		if (!::SetFilePointerEx(handle, seekOffset, nullptr, FILE_BEGIN)) {
			return InboxFileError{ .operation = "seek", .code = ::GetLastError() };
		}

		const auto bytesToRead = static_cast<DWORD>(byteCount);
		std::string bytes(bytesToRead, '\0');
		DWORD bytesRead = 0;
		if (!::ReadFile(handle, bytes.data(), bytesToRead, &bytesRead, nullptr)) {
			return InboxFileError{ .operation = "read", .code = ::GetLastError() };
		}

		bytes.resize(bytesRead);
		return std::move(bytes);
	}

	void InboxFile::Close() noexcept
	{
		if (_handle != nullptr) {
			::CloseHandle(static_cast<HANDLE>(_handle));
			_handle = nullptr;
		}
	}

	InboxFileReader::InboxFileReader(std::filesystem::path inboxPath) :
		_inboxPath(std::move(inboxPath))
	{}

	InboxFileResult InboxFileReader::Open() const noexcept
	{
		const auto handle = ::CreateFileW(
			_inboxPath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (handle == INVALID_HANDLE_VALUE) {
			const auto errorCode = ::GetLastError();
			if (errorCode == ERROR_FILE_NOT_FOUND || errorCode == ERROR_PATH_NOT_FOUND) {
				return InboxFileMissing{};
			}

			return InboxFileError{ .operation = "open", .code = errorCode };
		}

		FileObservation observation{};
		DWORD errorCode = ERROR_SUCCESS;
		if (!TryObserveOpenFile(handle, observation, errorCode)) {
			::CloseHandle(handle);
			return InboxFileError{ .operation = "observe", .code = errorCode };
		}

		return InboxFile(handle, observation);
	}
}
