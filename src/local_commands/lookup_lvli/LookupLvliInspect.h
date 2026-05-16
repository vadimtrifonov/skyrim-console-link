#pragma once

#include "local_commands/lookup_lvli/LookupLvli.h"

namespace RE
{
	class TESForm;
}

namespace LocalCommands
{
	[[nodiscard]] LookupLvliResult InspectLookupLvliForm(const RE::TESForm* form);
}
