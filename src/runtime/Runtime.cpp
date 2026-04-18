#include "runtime/Runtime.h"

#include "inbox/InputProcessor.h"
#include "logging/ConsoleActivityLog.h"
#include "observation/ConsoleInputObservation.h"
#include "observation/ConsoleOutputObservation.h"
#include "pch.h"

namespace
{
	constexpr std::size_t kObservationTrampolineSize = 128;

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

	void InitializeObservationTrampoline(const Settings::Values& settings)
	{
		if (!REL::Module::IsVR()) {
			return;
		}

		if (!settings.enableConsoleInputLogging && !settings.enableConsoleOutputLogging) {
			return;
		}

		try {
			SKSE::AllocTrampoline(kObservationTrampolineSize);
			logs::info("Reserved {} bytes of shared trampoline space for console observation hooks", kObservationTrampolineSize);
		} catch (const std::exception& exception) {
			logs::warn("Failed to reserve shared trampoline space for console observation hooks: {}", exception.what());
		} catch (...) {
			logs::warn("Failed to reserve shared trampoline space for console observation hooks");
		}
	}
}

namespace Runtime
{
	void Initialize(const Settings::Values& settings, const RuntimePaths& paths)
	{
		logs::info("Runtime scaffold initialized for {}", GetRuntimeName());
		logs::info(
			"Feature state input_logging={} output_logging={} inbox={}",
			settings.enableConsoleInputLogging,
			settings.enableConsoleOutputLogging,
			settings.enableInbox);

		if (const auto error = Logging::InitializeConsoleActivityLog()) {
			SKSE::stl::report_and_fail(*error);
		}
		InitializeObservationTrampoline(settings);

		if (!REL::Module::IsVR()) {
			logs::warn(
				"ConsoleLink is currently scaffolding a VR-first implementation; runtime-specific hook sites are not verified for {} yet",
				GetRuntimeName());
		}

		Observation::Output::Install(settings);
		Observation::Input::Install(settings);
		Inbox::Initialize(settings, paths.inboxPath);
	}
}
