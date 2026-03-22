#include "runtime/Runtime.h"

#include "capture/ConsoleInputCapture.h"
#include "capture/ConsoleOutputCapture.h"
#include "command/CommandSubmissionService.h"
#include "display/DisplayService.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

namespace
{
	constexpr std::size_t kCaptureTrampolineSize = 128;

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

	void InitializeCaptureTrampoline(const Config::Settings& settings)
	{
		if (!REL::Module::IsVR()) {
			return;
		}

		if (!settings.enableConsoleInputLogging && !settings.enableConsoleOutputLogging) {
			return;
		}

		try {
			SKSE::AllocTrampoline(kCaptureTrampolineSize);
			logs::info("Reserved {} bytes of shared trampoline space for console capture hooks", kCaptureTrampolineSize);
		} catch (const std::exception& exception) {
			logs::warn("Failed to reserve shared trampoline space for console capture hooks: {}", exception.what());
		} catch (...) {
			logs::warn("Failed to reserve shared trampoline space for console capture hooks");
		}
	}
}

namespace Runtime
{
	void Initialize(
		const Config::Settings&      settings,
		const std::filesystem::path& outboxPath,
		const std::filesystem::path& inboxPath)
	{
		logs::info("Runtime scaffold initialized for {}", GetRuntimeName());
		logs::info(
			"Feature state input_logging={} output_logging={} display={} command_submission={}",
			settings.enableConsoleInputLogging,
			settings.enableConsoleOutputLogging,
			settings.enableConsoleDisplay,
			settings.enableConsoleCommandSubmission);

		Logging::InitializeConsoleActivityLog(settings);
		InitializeCaptureTrampoline(settings);

		if (!REL::Module::IsVR()) {
			logs::warn(
				"ConsoleDebugger is currently scaffolding a VR-first implementation; runtime-specific hook sites are not verified for {} yet",
				GetRuntimeName());
		}

		Capture::Output::Install(settings);
		Capture::Input::Install(settings);
		Display::Initialize(settings, outboxPath);
		Command::Initialize(settings, inboxPath);
	}
}
