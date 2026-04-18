#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace RE
{
	class FxDelegateArgs;
}

namespace Console::ExecuteCommandSeam
{
	using AcceptedCommandHandler = void (*)(std::string_view command) noexcept;

	enum class InvokeResult
	{
		kInvoked,
		kTerminalFailure
	};

	[[nodiscard]] bool Validate() noexcept;
	[[nodiscard]] bool InstallAcceptedCommandHook(AcceptedCommandHandler handler) noexcept;
	[[nodiscard]] InvokeResult Invoke(const RE::FxDelegateArgs& args) noexcept;
}
