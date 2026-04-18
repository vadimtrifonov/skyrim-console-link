#pragma once

namespace Console::MenuState
{
	using StateChangeHandler = void (*)(bool isOpen) noexcept;

	void SetStateChangeHandler(StateChangeHandler handler) noexcept;
	[[nodiscard]] bool IsConsoleMenuOpen() noexcept;
}
