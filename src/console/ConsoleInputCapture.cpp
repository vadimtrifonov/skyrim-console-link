#include "console/ConsoleInputCapture.h"

#include "console/ExecuteCommandSeam.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

namespace
{
	std::atomic_bool g_captureDisabled = false;

	void OnAcceptedCommand(std::string_view command) noexcept
	{
		if (command.empty() || g_captureDisabled.load()) {
			return;
		}

		try {
			Logging::LogConsoleInputLine(command);
		} catch (const std::exception& exception) {
			g_captureDisabled = true;
			logs::warn("Disabling passive console input capture after failure: {}", exception.what());
		} catch (...) {
			g_captureDisabled = true;
			logs::warn("Disabling passive console input capture after failure: unknown exception");
		}
	}
}

namespace Console::InputCapture
{
	void Install(const Settings::Values& settings)
	{
		static bool attempted = false;
		if (attempted) {
			return;
		}
		attempted = true;

		if (!settings.enableConsoleInputLogging) {
			logs::info("Console input capture is disabled by settings");
			return;
		}

		if (!REL::Module::IsVR()) {
			logs::warn("Skipping passive console input capture: the ExecuteCommand seam is VR-only");
			return;
		}

		if (!Console::ExecuteCommandSeam::InstallAcceptedCommandHook(&OnAcceptedCommand)) {
			return;
		}
	}
}
