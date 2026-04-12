#include "bootstrap/Bootstrap.h"

#include "config/Settings.h"
#include "inbox/InboxPath.h"
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
			"Config input_logging={} output_logging={} inbox={} timestamps={}",
			loadResult.settings.enableConsoleInputLogging,
			loadResult.settings.enableConsoleOutputLogging,
			loadResult.settings.enableInbox,
			loadResult.settings.includeTimestamps);

		const auto configuredInboxPath = loadResult.settings.inboxPath;
		auto runtimeInboxPath = configuredInboxPath;
		const auto needsInboxResolution =
			loadResult.settings.enableInbox &&
			!configuredInboxPath.empty() &&
			!configuredInboxPath.is_absolute();

		std::optional<std::filesystem::path> logDirectory;
		if (needsInboxResolution) {
			logDirectory = logs::log_directory();
			if (!logDirectory) {
				SKSE::stl::report_and_fail("Failed to resolve the standard SKSE log directory for enabled relative inbox path resolution.");
			}
		}

		if (needsInboxResolution) {
			const auto resolvedInboxPath = Inbox::ResolveInboxPath(Inbox::InboxPathContext{
				.configuredPath = configuredInboxPath,
				.logDirectory = *logDirectory,
			});
			runtimeInboxPath = resolvedInboxPath.resolvedPath;
		}

		logs::info(
			"Config inbox_path='{}' resolved_inbox_path='{}' max_inbox_lines_per_tick={}",
			configuredInboxPath.string(),
			runtimeInboxPath.string(),
			loadResult.settings.maxInboxLinesPerTick);

		Runtime::Initialize(
			loadResult.settings,
			Runtime::RuntimePaths{
				.inboxPath = runtimeInboxPath,
			});
	}
}
