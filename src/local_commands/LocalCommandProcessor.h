#pragma once

#include <string_view>

namespace LocalCommands
{
	enum class ProcessResult
	{
		kHandled,
		kRejected
	};

	void Initialize() noexcept;
	[[nodiscard]] bool IsLocalCommandLine(std::string_view line) noexcept;
	[[nodiscard]] ProcessResult Process(std::string_view line) noexcept;
}
