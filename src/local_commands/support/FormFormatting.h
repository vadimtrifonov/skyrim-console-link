#pragma once

#include <RE/F/FormTypes.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace LocalCommands
{
	[[nodiscard]] inline std::string FormatFormID(const std::uint32_t formID)
	{
		std::ostringstream stream;
		stream << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << formID;
		return stream.str();
	}

	[[nodiscard]] inline std::string_view DescribeFormType(const RE::FormType formType) noexcept
	{
		switch (formType) {
		case RE::FormType::LeveledItem:
			return "LVLI";
		case RE::FormType::Weapon:
			return "WEAP";
		case RE::FormType::Armor:
			return "ARMO";
		case RE::FormType::NPC:
			return "NPC_";
		case RE::FormType::Outfit:
			return "OTFT";
		case RE::FormType::Keyword:
			return "KYWD";
		case RE::FormType::ConstructibleObject:
			return "COBJ";
		default:
			return "FORM";
		}
	}
}
