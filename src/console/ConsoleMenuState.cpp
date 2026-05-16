#include "console/ConsoleMenuState.h"

#include "pch.h"

#include "RE/C/Console.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/U/UI.h"

#include <vector>

namespace
{
	std::atomic_bool g_menuOpen = false;
	std::atomic_bool g_menuEventSinkRegistered = false;
	std::mutex g_stateChangeHandlersLock;
	std::vector<Console::MenuState::StateChangeHandler> g_stateChangeHandlers;

	void NotifyStateChange(const bool isOpen) noexcept
	{
		std::scoped_lock lock(g_stateChangeHandlersLock);
		if (g_stateChangeHandlers.empty()) {
			return;
		}

		const auto* taskInterface = SKSE::GetTaskInterface();
		if (taskInterface == nullptr) {
			logs::warn(
				"Console menu state changed, but its state-change handlers could not be scheduled because the SKSE task interface is unavailable");
			return;
		}

		for (const auto handler : g_stateChangeHandlers) {
			if (handler != nullptr) {
				taskInterface->AddTask([handler, isOpen] {
					handler(isOpen);
				});
			}
		}
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
				"Console menu state initialization could not register open/close events because RE::UI is unavailable");
			return;
		}

		ui->AddEventSink<RE::MenuOpenCloseEvent>(&GetMenuEventSink());
		g_menuOpen = ui->IsMenuOpen(RE::Console::MENU_NAME);
		g_menuEventSinkRegistered = true;
	}

	void AddStateChangeHandler(const StateChangeHandler handler) noexcept
	{
		if (handler == nullptr) {
			return;
		}

		try {
			std::scoped_lock lock(g_stateChangeHandlersLock);
			if (std::find(g_stateChangeHandlers.begin(), g_stateChangeHandlers.end(), handler) != g_stateChangeHandlers.end()) {
				return;
			}

			g_stateChangeHandlers.push_back(handler);
		} catch (const std::exception& exception) {
			logs::warn("Console menu state handler could not be registered: {}", exception.what());
			return;
		} catch (...) {
			logs::warn("Console menu state handler could not be registered: unknown exception");
			return;
		}

		Initialize();
	}

	bool IsConsoleMenuOpen() noexcept
	{
		Initialize();
		return g_menuOpen.load();
	}
}
