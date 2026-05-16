#include "logging/ConsoleActivityLog.h"
#include "logging/ConsoleActivityTaxonomy.h"

#include <cstring>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace
{
	void Expect(bool condition, std::string_view message)
	{
		if (!condition) {
			throw std::runtime_error(std::string(message));
		}
	}

	void TestCanonicalChannels()
	{
		using Logging::ConsoleActivityKind;

		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kCapturedConsoleOutput) == "console-out",
			"captured console output should use console-out");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kCapturedConsoleInput) == "console-in",
			"captured console input should use console-in");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kSubmittedConsoleCommand) == "console-submit",
			"submitted console commands should use console-submit");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kLocalActivity) == "local",
			"local activity should use local");
		Expect(
			Logging::GetConsoleActivityLogFilename() == "ConsoleLink.activity.log",
			"activity log filename should use the canonical artifact");
	}

	void TestFormattedRecords()
	{
		using Logging::ConsoleActivityKind;

		Expect(
			Logging::BuildConsoleActivityRecord(ConsoleActivityKind::kLocalActivity, "unknown local command") ==
				"[local] unknown local command",
			"local activity should use the canonical record format");
	}

	void TestUnknownActivityKindFailsFast()
	{
		using ActivityKind = Logging::ConsoleActivityKind;
		using ActivityKindValue = std::underlying_type_t<ActivityKind>;

		const ActivityKindValue invalidValue = 999;
		ActivityKind invalidKind{};
		std::memcpy(&invalidKind, &invalidValue, sizeof(invalidKind));

		auto threw = false;
		try {
			static_cast<void>(Logging::DescribeConsoleActivityChannel(invalidKind));
		} catch (const std::invalid_argument&) {
			threw = true;
		}

		Expect(threw, "unknown activity kinds should fail fast");
	}
}

void RunConsoleActivityTaxonomyTests()
{
	TestCanonicalChannels();
	TestFormattedRecords();
	TestUnknownActivityKindFailsFast();
}
