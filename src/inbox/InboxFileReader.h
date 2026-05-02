#pragma once

#include "inbox/InboxFileObservation.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>

namespace Inbox
{
	struct InboxFileMissing
	{};

	struct InboxFileError
	{
		std::string_view operation;
		std::uint32_t code = 0;
	};

	using InboxFileReadResult = std::variant<std::string, InboxFileError>;

	class InboxFile
	{
	public:
		InboxFile() noexcept = default;
		~InboxFile() noexcept;

		InboxFile(const InboxFile&) = delete;
		InboxFile& operator=(const InboxFile&) = delete;
		InboxFile(InboxFile&& other) noexcept;
		InboxFile& operator=(InboxFile&& other) noexcept;

		[[nodiscard]] const FileObservation& Observation() const noexcept;
		[[nodiscard]] InboxFileReadResult ReadAt(std::uint64_t offset, std::uint64_t byteCount) const noexcept;

	private:
		friend class InboxFileReader;

		InboxFile(void* handle, FileObservation observation) noexcept;
		void Close() noexcept;

		void* _handle = nullptr;
		FileObservation _observation{};
	};

	using InboxFileResult = std::variant<InboxFile, InboxFileMissing, InboxFileError>;

	class InboxFileReader
	{
	public:
		explicit InboxFileReader(std::filesystem::path inboxPath);

		[[nodiscard]] InboxFileResult Open() const noexcept;

	private:
		std::filesystem::path _inboxPath;
	};
}
