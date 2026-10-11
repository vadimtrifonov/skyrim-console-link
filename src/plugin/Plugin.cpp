#include "plugin/Plugin.h"

#include "console/ConsoleInputCapture.h"
#include "console/ConsoleOutputCapture.h"
#include "console_commands/ConsoleCommandProcessor.h"
#include "inbox/InboxPath.h"
#include "inbox/InputProcessor.h"
#include "local_commands/LocalCommandProcessor.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"
#include "settings/Settings.h"

namespace
{
	Settings::Values g_settings;

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

	void InitializeSKSE(const SKSE::LoadInterface* skse, const Settings::Values& settings)
	{
		const auto requiredSize = Console::OutputCapture::RequiredTrampolineSize(settings);
		// Keep the ConsoleLink.log logger configured by the entry point.
		SKSE::Init(skse, SKSE::InitInfo{
							 .log = false,
							 .trampoline = requiredSize != 0,
							 .trampolineSize = requiredSize,
						 });

		if (requiredSize == 0) {
			return;
		}

		try {
			auto& trampoline = SKSE::GetTrampoline();
			if (trampoline.capacity() == 0) {
				// InitInfo does not allocate when the host lacks a trampoline interface.
				trampoline.create(requiredSize);
			}
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

	void OnDataLoaded()
	{
		logs::info("Initializing data-loaded services");

		Console::InputCapture::Initialize(g_settings);

		if (g_settings.enableInbox) {
			ConsoleCommands::Initialize();
			LocalCommands::Initialize();
			Inbox::StartProcessing();
		}

		logs::info("Data-loaded services initialized");
	}

	void HandleMessage(SKSE::MessagingInterface::Message* message) noexcept
	{
		if (message == nullptr || message->type != SKSE::MessagingInterface::kDataLoaded) {
			return;
		}

		try {
			OnDataLoaded();
		} catch (const std::exception& exception) {
			try {
				logs::critical("Failed to initialize data-loaded services: {}", exception.what());
			} catch (...) {
			}
		} catch (...) {
			try {
				logs::critical("Failed to initialize data-loaded services: unknown exception");
			} catch (...) {
			}
		}
	}

	void RegisterMessageListener()
	{
		const auto* messaging = SKSE::GetMessagingInterface();
		if (messaging == nullptr) {
			SKSE::stl::report_and_fail("Failed to obtain the SKSE messaging interface.");
		}

		if (!messaging->RegisterListener(HandleMessage)) {
			SKSE::stl::report_and_fail("Failed to register the ConsoleLink SKSE messaging listener.");
		}
	}
}

namespace ConsoleLink::Plugin
{
	void Load(const SKSE::LoadInterface* skse)
	{
		logs::info("Initializing ConsoleLink on {} runtime {}", GetRuntimeName(), REL::Module::get().version());

		auto loadResult = LoadSettings();
		InitializeSKSE(skse, loadResult.settings);
		auto inboxPath = ResolvePath(loadResult.settings);

		logs::info(
			"Settings inbox_path='{}' resolved_inbox_path='{}' max_inbox_lines_per_tick={}",
			loadResult.settings.inboxPath.string(),
			inboxPath.string(),
			loadResult.settings.maxInboxLinesPerTick);

		if (const auto result = Logging::InitializeConsoleActivityLog(); !result) {
			SKSE::stl::report_and_fail(result.error());
		}

		Inbox::Initialize(loadResult.settings, inboxPath);
		g_settings = std::move(loadResult.settings);

		RegisterMessageListener();
		Console::OutputCapture::Install(g_settings);
	}
}
