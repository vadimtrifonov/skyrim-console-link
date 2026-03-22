#include "bootstrap/Bootstrap.h"

#include "command/InboxPath.h"
#include "config/Settings.h"
#include "display/OutboxPath.h"
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
		logs::info("Initializing ConsoleLink on {} runtime {}", GetRuntimeName(), REL::Module::get().version());

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
			"Config input_logging={} output_logging={} display={} command_submission={} separate_console_log={} timestamps={}",
			loadResult.settings.enableConsoleInputLogging,
			loadResult.settings.enableConsoleOutputLogging,
			loadResult.settings.enableConsoleDisplay,
			loadResult.settings.enableConsoleCommandSubmission,
			loadResult.settings.mirrorToSeparateConsoleLog,
			loadResult.settings.includeTimestamps);

		const auto configuredOutboxPath = loadResult.settings.outboxPath;
		auto       runtimeOutboxPath = configuredOutboxPath;
		const auto configuredInboxPath = loadResult.settings.inboxPath;
		auto       runtimeInboxPath = configuredInboxPath;
		const auto needsOutboxResolution =
			loadResult.settings.enableConsoleDisplay &&
			!configuredOutboxPath.empty() &&
			!configuredOutboxPath.is_absolute();
		const auto needsInboxResolution =
			loadResult.settings.enableConsoleCommandSubmission &&
			!configuredInboxPath.empty() &&
			!configuredInboxPath.is_absolute();

		std::optional<std::filesystem::path> logDirectory;
		if (needsOutboxResolution || needsInboxResolution) {
			logDirectory = logs::log_directory();
			if (!logDirectory) {
				SKSE::stl::report_and_fail("Failed to resolve the standard SKSE log directory for enabled file-backed console path resolution.");
			}
		}

		if (needsOutboxResolution) {
			const auto resolvedOutboxPath = Display::ResolveOutboxPath(configuredOutboxPath, *logDirectory);
			if (resolvedOutboxPath.remappedLegacyDefault) {
				logs::warn(
					"Remapping legacy display outbox path '{}' to the SKSE log directory default '{}'",
					configuredOutboxPath.string(),
					resolvedOutboxPath.resolvedPath.string());
			}
			runtimeOutboxPath = resolvedOutboxPath.resolvedPath;
		}

		if (needsInboxResolution) {
			runtimeInboxPath = Command::ResolveInboxPath(configuredInboxPath, *logDirectory).resolvedPath;
		}

		logs::info(
			"Config display_outbox_path='{}' resolved_display_outbox_path='{}' max_display_lines_per_tick={} inbox_path='{}' resolved_inbox_path='{}' max_commands_per_tick={} trace_hook_install={}",
			configuredOutboxPath.string(),
			runtimeOutboxPath.string(),
			loadResult.settings.maxDisplayLinesPerTick,
			configuredInboxPath.string(),
			runtimeInboxPath.string(),
			loadResult.settings.maxCommandsPerTick,
			loadResult.settings.traceHookInstall);

		Runtime::Initialize(loadResult.settings, runtimeOutboxPath, runtimeInboxPath);
	}
}

