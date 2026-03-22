#include "bootstrap/Bootstrap.h"

#include "config/Settings.h"
#include "pch.h"
#include "runtime/Runtime.h"
#include "writeback/OutboxPath.h"

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

		auto loadResult = Config::LoadSettingsWithDiagnostics(settingsPath);
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

		const auto configuredOutboxPath = loadResult.settings.outboxPath;
		auto       runtimeOutboxPath = configuredOutboxPath;
		if (!configuredOutboxPath.empty()) {
			auto logDirectory = logs::log_directory();
			if (!logDirectory) {
				SKSE::stl::report_and_fail("Failed to resolve the standard SKSE log directory for outbox path resolution.");
			}

			const auto resolvedOutboxPath = WriteBack::ResolveOutboxPath(configuredOutboxPath, *logDirectory);
			if (resolvedOutboxPath.remappedLegacyDefault) {
				logs::warn(
					"Remapping legacy outbox path '{}' to the SKSE log directory default '{}'",
					configuredOutboxPath.string(),
					resolvedOutboxPath.resolvedPath.string());
			}
			runtimeOutboxPath = resolvedOutboxPath.resolvedPath;
		}

		logs::info(
			"Config outbox_path='{}' resolved_outbox_path='{}' max_lines_per_tick={} trace_hook_install={}",
			configuredOutboxPath.string(),
			runtimeOutboxPath.string(),
			loadResult.settings.maxLinesPerTick,
			loadResult.settings.traceHookInstall);

		Runtime::Initialize(loadResult.settings, runtimeOutboxPath);
	}
}
