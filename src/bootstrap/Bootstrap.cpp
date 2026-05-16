#include "bootstrap/Bootstrap.h"

#include "console/ConsoleInputCapture.h"
#include "console/ConsoleOutputCapture.h"
#include "inbox/InboxPath.h"
#include "inbox/InputProcessor.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"
#include "settings/Settings.h"

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

	void InitializeOutputCaptureTrampoline(const Settings::Values& settings)
	{
		const auto requiredSize = Console::OutputCapture::RequiredTrampolineSize(settings);
		if (requiredSize == 0) {
			return;
		}

		try {
			SKSE::AllocTrampoline(requiredSize);
			logs::info("Reserved {} bytes of trampoline space for console output capture", requiredSize);
		} catch (const std::exception& exception) {
			logs::warn("Failed to reserve trampoline space for console output capture: {}", exception.what());
		} catch (...) {
			logs::warn("Failed to reserve trampoline space for console output capture");
		}
	}

	std::filesystem::path ResolvePath(const Settings::Values& settings)
	{
		const auto configuredInboxPath = settings.inboxPath;
		if (!settings.enableInbox || configuredInboxPath.empty() || configuredInboxPath.is_absolute()) {
			return configuredInboxPath;
		}

		const auto logDirectory = logs::log_directory();
		if (!logDirectory) {
			SKSE::stl::report_and_fail("Failed to resolve the standard SKSE log directory for enabled relative inbox path resolution.");
		}

		const auto resolvedInboxPath = Inbox::ResolvePath(Inbox::PathContext{
			.configuredPath = configuredInboxPath,
			.logDirectory = *logDirectory,
		});
		return resolvedInboxPath.resolvedPath;
	}

	Settings::LoadResult LoadSettings()
	{
		const auto settingsPath = Settings::GetDefaultSettingsPath();
		if (std::filesystem::exists(settingsPath)) {
			logs::info("Loading settings from {}", settingsPath.string());
		} else {
			logs::warn("Settings file not found at {}; using built-in defaults", settingsPath.string());
		}

		auto loadResult = Settings::LoadSettingsWithDiagnostics(settingsPath);
		for (const auto& warning : loadResult.warnings) {
			logs::warn("{}", warning);
		}

		logs::info(
			"Settings input_logging={} output_logging={} inbox={}",
			loadResult.settings.enableConsoleInputLogging,
			loadResult.settings.enableConsoleOutputLogging,
			loadResult.settings.enableInbox);

		return loadResult;
	}
}

namespace Bootstrap
{
	void Initialize()
	{
		logs::info("Initializing ConsoleLink on {} runtime {}", GetRuntimeName(), REL::Module::get().version());

		auto loadResult = LoadSettings();
		auto inboxPath = ResolvePath(loadResult.settings);

		logs::info(
			"Settings inbox_path='{}' resolved_inbox_path='{}' max_inbox_lines_per_tick={}",
			loadResult.settings.inboxPath.string(),
			inboxPath.string(),
			loadResult.settings.maxInboxLinesPerTick);

		if (const auto result = Logging::InitializeConsoleActivityLog(); !result) {
			SKSE::stl::report_and_fail(result.error());
		}
		InitializeOutputCaptureTrampoline(loadResult.settings);

		Console::InputCapture::Initialize(loadResult.settings);
		Console::OutputCapture::Install(loadResult.settings);
		Inbox::Initialize(loadResult.settings, inboxPath);
	}
}
