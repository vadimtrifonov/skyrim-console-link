#include "bootstrap/Bootstrap.h"

#include "command/InboxPath.h"
#include "config/Settings.h"
#include "pch.h"
#include "print/OutboxPath.h"
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
			"Config input_logging={} output_logging={} print={} command_submission={} timestamps={}",
			loadResult.settings.enableConsoleInputLogging,
			loadResult.settings.enableConsoleOutputLogging,
			loadResult.settings.enableConsolePrint,
			loadResult.settings.enableConsoleCommandSubmission,
			loadResult.settings.includeTimestamps);

		const auto configuredOutboxPath = loadResult.settings.outboxPath;
		auto       runtimeOutboxPath = configuredOutboxPath;
		const auto configuredInboxPath = loadResult.settings.inboxPath;
		auto       runtimeInboxPath = configuredInboxPath;
		const auto needsOutboxResolution =
			loadResult.settings.enableConsolePrint &&
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
			const auto resolvedOutboxPath = Print::ResolveOutboxPath(Print::OutboxPathContext{
				.configuredPath = configuredOutboxPath,
				.logDirectory = *logDirectory });
			runtimeOutboxPath = resolvedOutboxPath.resolvedPath;
		}

		if (needsInboxResolution) {
			const auto resolvedInboxPath = Command::ResolveInboxPath(Command::InboxPathContext{
				.configuredPath = configuredInboxPath,
				.logDirectory = *logDirectory,
			});
			runtimeInboxPath = resolvedInboxPath.resolvedPath;
		}

		logs::info(
			"Config print_outbox_path='{}' resolved_print_outbox_path='{}' max_print_lines_per_tick={} inbox_path='{}' resolved_inbox_path='{}' max_commands_per_tick={}",
			configuredOutboxPath.string(),
			runtimeOutboxPath.string(),
			loadResult.settings.maxPrintLinesPerTick,
			configuredInboxPath.string(),
			runtimeInboxPath.string(),
			loadResult.settings.maxCommandsPerTick);

		Runtime::Initialize(
			loadResult.settings,
			Runtime::RuntimePaths{
				.outboxPath = runtimeOutboxPath,
				.inboxPath = runtimeInboxPath,
			});
	}
}
