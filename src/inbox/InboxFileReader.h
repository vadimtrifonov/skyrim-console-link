#pragma once

#include "inbox/InboxFileObservation.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>

namespace Inbox
{
	struct FileMissing
	{};

	struct FileError
	{
		std::string_view operation;
		std::uint32_t code = 0;
	};

	using FileReadResult = std::variant<std::string, FileError>;

	class OpenFile
	{
	public:
		OpenFile() noexcept = default;
		~OpenFile() noexcept;

		OpenFile(const OpenFile&) = delete;
		OpenFile& operator=(const OpenFile&) = delete;
		OpenFile(OpenFile&& other) noexcept;
		OpenFile& operator=(OpenFile&& other) noexcept;

		[[nodiscard]] const FileObservation& Observation() const noexcept;
		[[nodiscard]] FileReadResult ReadAt(std::uint64_t offset, std::uint64_t byteCount) const noexcept;

	private:
		friend class FileReader;

		OpenFile(void* handle, FileObservation observation) noexcept;
		void Close() noexcept;

		void* _handle = nullptr;
		FileObservation _observation{};
	};

	using OpenResult = std::variant<OpenFile, FileMissing, FileError>;

	class FileReader
	{
	public:
		explicit FileReader(std::filesystem::path inboxPath);

		[[nodiscard]] OpenResult Open() const noexcept;

	private:
		std::filesystem::path _inboxPath;
	};
}
