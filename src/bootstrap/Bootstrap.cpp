#include "bootstrap/Bootstrap.h"

#include "config/Settings.h"
#include "pch.h"
#include "runtime/Runtime.h"

namespace
{
	std::string_view GetRuntimeName()
	{
		if (REL::Module::IsVR()) {
			return "Skyrim VR";
		}
		if (REL::Module::IsAE()) {
			return "Skyrim AE";
		}
		return "Skyrim SE";
	}
}

namespace Bootstrap
{
	void Initialize()
	{
		logs::info("Initializing ConsoleDebugger on {} runtime {}", GetRuntimeName(), REL::Module::get().version());

		const auto settingsPath = Config::GetDefaultSettingsPath();
		const auto settingsExists = std::filesystem::exists(settingsPath);
		if (settingsExists) {
			logs::info("Loading config from {}", settingsPath.string());
		} else {
			logs::warn("Config file not found at {}; using built-in defaults", settingsPath.string());
		}

		const auto loadResult = Config::LoadSettingsWithDiagnostics(settingsPath);
		for (const auto& warning : loadResult.warnings) {
			logs::warn("{}", warning);
		}

		logs::info(
			"Config input_logging={} output_logging={} write_back={} separate_console_log={} timestamps={}",
			loadResult.settings.enableConsoleInputLogging,
			loadResult.settings.enableConsoleOutputLogging,
			loadResult.settings.enableConsoleWriteBack,
			loadResult.settings.mirrorToSeparateConsoleLog,
			loadResult.settings.includeTimestamps);
		logs::info(
			"Config outbox_path='{}' max_lines_per_tick={} trace_hook_install={}",
			loadResult.settings.outboxPath.string(),
			loadResult.settings.maxLinesPerTick,
			loadResult.settings.traceHookInstall);

		Runtime::Initialize(loadResult.settings);
	}
}
