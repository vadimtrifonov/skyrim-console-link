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
	constexpr std::size_t kConsoleTrampolineSize = 128;

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

	void InitializeConsoleTrampoline(const Settings::Values& settings)
	{
		if (!REL::Module::IsVR()) {
			logs::warn(
				"ConsoleLink is currently scaffolding a VR-first implementation; runtime-specific hook sites are not verified for {} yet",
				GetRuntimeName());
			return;
		}

		if (!settings.enableConsoleInputLogging && !settings.enableConsoleOutputLogging) {
			return;
		}

		try {
			SKSE::AllocTrampoline(kConsoleTrampolineSize);
			logs::info("Reserved {} bytes of shared trampoline space for console integration hooks", kConsoleTrampolineSize);
		} catch (const std::exception& exception) {
			logs::warn("Failed to reserve shared trampoline space for console integration hooks: {}", exception.what());
		} catch (...) {
			logs::warn("Failed to reserve shared trampoline space for console integration hooks");
		}
	}

	std::filesystem::path ResolveInboxPath(const Settings::Values& settings)
	{
		const auto configuredInboxPath = settings.inboxPath;
		if (!settings.enableInbox || configuredInboxPath.empty() || configuredInboxPath.is_absolute()) {
			return configuredInboxPath;
		}

		const auto logDirectory = logs::log_directory();
		if (!logDirectory) {
			SKSE::stl::report_and_fail("Failed to resolve the standard SKSE log directory for enabled relative inbox path resolution.");
		}

		const auto resolvedInboxPath = Inbox::ResolveInboxPath(Inbox::InboxPathContext{
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
		auto inboxPath = ResolveInboxPath(loadResult.settings);

		logs::info(
			"Settings inbox_path='{}' resolved_inbox_path='{}' max_inbox_lines_per_tick={}",
			loadResult.settings.inboxPath.string(),
			inboxPath.string(),
			loadResult.settings.maxInboxLinesPerTick);

		if (const auto result = Logging::InitializeConsoleActivityLog(); !result) {
			SKSE::stl::report_and_fail(result.error());
		}
		InitializeConsoleTrampoline(loadResult.settings);

		Console::InputCapture::Install(loadResult.settings);
		Console::OutputCapture::Install(loadResult.settings);
		Inbox::Initialize(loadResult.settings, inboxPath);
	}
}
