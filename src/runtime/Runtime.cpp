#include "runtime/Runtime.h"

#include "logging/ConsoleActivityLog.h"
#include "pch.h"
#include "writeback/WriteBackService.h"

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

namespace Runtime
{
	void Initialize(const Config::Settings& settings, const std::filesystem::path& outboxPath)
	{
		logs::info("Runtime scaffold initialized for {}", GetRuntimeName());
		logs::info(
			"Feature state input_logging={} output_logging={} write_back={}",
			settings.enableConsoleInputLogging,
			settings.enableConsoleOutputLogging,
			settings.enableConsoleWriteBack);

		Logging::InitializeConsoleActivityLog(settings);

		if (!REL::Module::IsVR()) {
			logs::warn(
				"ConsoleDebugger is currently scaffolding a VR-first implementation; runtime-specific hook sites are not verified for {} yet",
				GetRuntimeName());
		}

		WriteBack::Initialize(settings, outboxPath);
		logs::info("Observational console hooks are not installed in this build");
	}
}
