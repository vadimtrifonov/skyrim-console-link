#pragma once

#include <string_view>

namespace Command
{
	enum class LocalCommandDispatchResult
	{
		kNotHandled,
		kHandled,
		kDeferred,
		kTerminalFailure
	};

	[[nodiscard]] LocalCommandDispatchResult TryDispatchLocalCommand(std::string_view commandLine, bool dataLoaded) noexcept;
}
