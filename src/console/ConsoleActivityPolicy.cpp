#include "console/ConsoleActivityPolicy.h"

#include "pch.h"

namespace
{
	thread_local std::uint32_t g_consoleOutputSuppressionDepth = 0;
}

namespace Console
{
	SuppressOutputScope::SuppressOutputScope()
	{
		++g_consoleOutputSuppressionDepth;
	}

	SuppressOutputScope::~SuppressOutputScope()
	{
		if (g_consoleOutputSuppressionDepth > 0) {
			--g_consoleOutputSuppressionDepth;
		}
	}

	bool IsOutputSuppressed()
	{
		return g_consoleOutputSuppressionDepth != 0;
	}
}
