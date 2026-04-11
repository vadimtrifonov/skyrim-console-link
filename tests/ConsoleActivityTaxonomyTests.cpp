#include "logging/ConsoleActivityTaxonomy.h"

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

	void TestCanonicalChannels()
	{
		using Logging::ConsoleActivityKind;

		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kObservedConsoleOutput) == "console-out",
			"observed console output should use console-out");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kObservedConsoleInput) == "console-in",
			"observed console input should use console-in");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kSubmittedConsoleCommand) == "console-submit",
			"submitted console commands should use console-submit");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kLocalCommandActivity) == "local",
			"local command activity should use local");
		Expect(
			Logging::DescribeConsoleActivityChannel(ConsoleActivityKind::kPrintedConsoleLine) == "console-print",
			"printed console lines should use console-print");
	}

	void TestFormattedRecords()
	{
		using Logging::ConsoleActivityKind;

		Expect(
			Logging::BuildConsoleActivityRecord(ConsoleActivityKind::kPrintedConsoleLine, "ConsoleLink: status ready") ==
				"[console-print] ConsoleLink: status ready",
			"printed console lines should use the canonical record format");
		Expect(
			Logging::BuildConsoleActivityRecord(ConsoleActivityKind::kLocalCommandActivity, "unknown local command") ==
				"[local] unknown local command",
			"local command activity should use the canonical record format");
	}

	void TestUnknownActivityKindFailsFast()
	{
		auto threw = false;
		try {
			static_cast<void>(Logging::DescribeConsoleActivityChannel(static_cast<Logging::ConsoleActivityKind>(999)));
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
