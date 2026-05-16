#pragma once

#include <string_view>

namespace ConsoleCommands
{
	enum class ProcessResult
	{
		kHandled,
		kRejected
	};

	void Initialize() noexcept;
	[[nodiscard]] ProcessResult Process(std::string_view line) noexcept;
}
