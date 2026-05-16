#pragma once

namespace Console::MenuState
{
	using StateChangeHandler = void (*)(bool isOpen) noexcept;

	void AddStateChangeHandler(StateChangeHandler handler) noexcept;
	[[nodiscard]] bool IsConsoleMenuOpen() noexcept;
}
