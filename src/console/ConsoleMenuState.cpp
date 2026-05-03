#include "console/ConsoleMenuState.h"

#include "pch.h"

#include "RE/C/Console.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/U/UI.h"

namespace
{
	std::atomic_bool g_menuOpen = false;
	std::atomic_bool g_menuEventSinkRegistered = false;
	std::atomic<Console::MenuState::StateChangeHandler> g_stateChangeHandler = nullptr;

	void NotifyStateChange(const bool isOpen) noexcept
	{
		const auto handler = g_stateChangeHandler.load();
		if (handler == nullptr) {
			return;
		}

		const auto* taskInterface = SKSE::GetTaskInterface();
		if (taskInterface == nullptr) {
			logs::warn(
				"Console menu state changed, but its state-change handler could not be scheduled because the SKSE task interface is unavailable");
			return;
		}

		taskInterface->AddTask([handler, isOpen] {
			handler(isOpen);
		});
	}

	class MenuEventSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::MenuOpenCloseEvent* event,
			RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			if (event == nullptr || event->menuName != RE::Console::MENU_NAME) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto previous = g_menuOpen.exchange(event->opening);
			if (previous != event->opening) {
				NotifyStateChange(event->opening);
			}

			return RE::BSEventNotifyControl::kContinue;
		}
	};

	MenuEventSink& GetMenuEventSink()
	{
		static MenuEventSink sink;
		return sink;
	}
}

namespace Console::MenuState
{
	void Initialize() noexcept
	{
		if (g_menuEventSinkRegistered.load()) {
			return;
		}

		auto* ui = RE::UI::GetSingleton();
		if (ui == nullptr) {
			logs::warn(
				"Console menu state initialization could not register open/close events because RE::UI is not available yet");
			return;
		}

		ui->AddEventSink<RE::MenuOpenCloseEvent>(&GetMenuEventSink());
		g_menuOpen = ui->IsMenuOpen(RE::Console::MENU_NAME);
		g_menuEventSinkRegistered = true;
	}

	void SetStateChangeHandler(const StateChangeHandler handler) noexcept
	{
		g_stateChangeHandler = handler;
		Initialize();
	}

	bool IsConsoleMenuOpen() noexcept
	{
		Initialize();
		return g_menuOpen.load();
	}
}
