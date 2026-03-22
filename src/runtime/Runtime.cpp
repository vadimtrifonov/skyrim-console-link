#include "runtime/Runtime.h"

#include "pch.h"

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
	void Initialize(const Config::Settings& settings)
	{
		logs::info("Runtime scaffold initialized for {}", GetRuntimeName());
		logs::info(
			"Feature state input_logging={} output_logging={} write_back={}",
			settings.enableConsoleInputLogging,
			settings.enableConsoleOutputLogging,
			settings.enableConsoleWriteBack);

		if (!REL::Module::IsVR()) {
			logs::warn(
				"ConsoleDebugger is currently scaffolding a VR-first implementation; runtime-specific hook sites are not verified for {} yet",
				GetRuntimeName());
		}

		logs::info("No observational hooks or write-back loop are installed in this scaffold build");
	}
}
