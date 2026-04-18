#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace LocalCommands
{
	enum class LookupLvliResultKind : std::uint8_t
	{
		kMiss,
		kWrongType,
		kHit
	};

	struct LookupLvliEntry
	{
		std::uint16_t level = 0;
		std::uint16_t count = 0;
		std::uint32_t formID = 0;
		std::string formType;
		std::string editorID;
		std::string name;
		std::string source;
	};

	struct LookupLvliHit
	{
		std::uint32_t formID = 0;
		std::string source;
		std::vector<LookupLvliEntry> entries;
	};

	struct LookupLvliWrongType
	{
		std::uint32_t formID = 0;
		std::string formType;
		std::string source;
	};

	struct LookupLvliResult
	{
		LookupLvliResultKind kind = LookupLvliResultKind::kMiss;
		LookupLvliWrongType wrongType;
		LookupLvliHit hit;
	};

	[[nodiscard]] std::vector<std::string> ExecuteLookupLvliCommand(std::string_view arguments);
	[[nodiscard]] std::vector<std::string> ExecuteLookupLvliCommand(
		std::string_view arguments,
		const LookupLvliResult& result);
}
