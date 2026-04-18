#include "local_commands/support/FormKey.h"

#include <stdexcept>
#include <string_view>

namespace
{
	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	void TestBuildRuntimeFormIDForRegularPlugin()
	{
		const auto runtimeFormID = LocalCommands::BuildRuntimeFormID(
			LocalCommands::PluginLoadAddress{
				.compileIndex = 0x02,
				.smallFileCompileIndex = 0,
				.isLight = false,
			},
			0x0003DF19);

		Expect(runtimeFormID.has_value(), "regular plugin runtime form ID should resolve");
		Expect(*runtimeFormID == 0x0203DF19, "regular plugin runtime form ID should include the compile index");
	}

	void TestBuildRuntimeFormIDForLightPlugin()
	{
		const auto runtimeFormID = LocalCommands::BuildRuntimeFormID(
			LocalCommands::PluginLoadAddress{
				.compileIndex = 0xFE,
				.smallFileCompileIndex = 0x041B,
				.isLight = true,
			},
			0x00000DFD);

		Expect(runtimeFormID.has_value(), "light plugin runtime form ID should resolve");
		Expect(*runtimeFormID == 0xFE41BDFD, "light plugin runtime form ID should preserve the ESL subindex");
	}

	void TestBuildRuntimeFormIDRejectsUnloadedPlugin()
	{
		const auto runtimeFormID = LocalCommands::BuildRuntimeFormID(
			LocalCommands::PluginLoadAddress{
				.compileIndex = 0xFF,
				.smallFileCompileIndex = 0,
				.isLight = false,
			},
			0x0003DF19);

		Expect(!runtimeFormID.has_value(), "unloaded plugin should not produce a runtime form ID");
	}
}

void RunFormKeyTests()
{
	TestBuildRuntimeFormIDForRegularPlugin();
	TestBuildRuntimeFormIDForLightPlugin();
	TestBuildRuntimeFormIDRejectsUnloadedPlugin();
}
