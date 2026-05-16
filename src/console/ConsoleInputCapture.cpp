#include "console/ConsoleInputCapture.h"

#include "console/ConsoleMenuState.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include "RE/C/Console.h"
#include "RE/F/FxDelegate.h"
#include "RE/F/FxDelegateArgs.h"
#include "RE/U/UI.h"

namespace
{
	using CallbackFn = RE::FxDelegateHandler::CallbackFn;

	std::mutex g_originalExecuteCommandCallbackLock;
	CallbackFn* g_originalExecuteCommandCallback = nullptr;

	std::string TrimTrailingLineTerminators(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}
		return value;
	}

	std::optional<std::string> ExtractCommandText(const RE::FxDelegateArgs& args)
	{
		if (args.GetArgCount() == 0) {
			return std::nullopt;
		}

		const auto& value = args[0];
		if (!value.IsString()) {
			return std::nullopt;
		}

		const auto* command = value.GetString();
		if (command == nullptr) {
			return std::nullopt;
		}

		return TrimTrailingLineTerminators(command);
	}

	void CaptureExecuteCommandCallback(const RE::FxDelegateArgs& args)
	{
		std::string command;
		try {
			if (auto extracted = ExtractCommandText(args); extracted.has_value()) {
				command = std::move(*extracted);
			}
		} catch (const std::exception& exception) {
			logs::warn("Skipping passive console input capture for this command after extraction failure: {}", exception.what());
		} catch (...) {
			logs::warn("Skipping passive console input capture for this command after an unknown extraction failure");
		}

		CallbackFn* originalCallback = nullptr;
		{
			std::scoped_lock lock(g_originalExecuteCommandCallbackLock);
			originalCallback = g_originalExecuteCommandCallback;
		}
		if (originalCallback == nullptr) {
			SKSE::stl::report_and_fail("ConsoleLink input capture invariant failed: original ExecuteCommand callback is unavailable");
		}

		originalCallback(args);
		if (!command.empty()) {
			Logging::LogConsoleInputLine(command);
		}
	}

	bool TryWrapActiveConsoleMenu() noexcept
	{
		auto* ui = RE::UI::GetSingleton();
		if (ui == nullptr) {
			logs::warn("Skipping passive console input capture: RE::UI is unavailable");
			return false;
		}

		auto console = ui->GetMenu<RE::Console>();
		if (!console) {
			return false;
		}

		auto* delegate = console->fxDelegate.get();
		if (delegate == nullptr) {
			logs::warn("Skipping passive console input capture for the open console menu: FxDelegate is unavailable");
			return false;
		}

		auto* callback = delegate->callbacks.GetAlt("ExecuteCommand");
		if (callback == nullptr || callback->callback == nullptr) {
			logs::warn("Skipping passive console input capture for the open console menu: ExecuteCommand callback is unavailable");
			return false;
		}

		std::scoped_lock lock(g_originalExecuteCommandCallbackLock);
		if (callback->callback == &CaptureExecuteCommandCallback) {
			return true;
		}

		if (g_originalExecuteCommandCallback == nullptr) {
			g_originalExecuteCommandCallback = callback->callback;
		} else if (g_originalExecuteCommandCallback != callback->callback) {
			logs::warn(
				"Skipping passive console input capture for the open console menu: ExecuteCommand callback changed after ConsoleLink installed its wrapper");
			return false;
		}

		callback->callback = &CaptureExecuteCommandCallback;
		logs::info("Wrapped active console FxDelegate ExecuteCommand callback for passive input capture");
		return true;
	}

	void OnMenuStateChange(const bool isOpen) noexcept
	{
		if (isOpen) {
			static_cast<void>(TryWrapActiveConsoleMenu());
		}
	}
}

namespace Console::InputCapture
{
	void Initialize(const Settings::Values& settings)
	{
		static bool initialized = false;
		if (initialized) {
			return;
		}
		initialized = true;

		if (!settings.enableConsoleInputLogging) {
			logs::info("Console input capture is disabled by settings");
			return;
		}

		Console::MenuState::AddStateChangeHandler(&OnMenuStateChange);
		static_cast<void>(TryWrapActiveConsoleMenu());
	}
}
