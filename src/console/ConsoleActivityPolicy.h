#pragma once

namespace Console
{
	class SuppressOutputScope
	{
	public:
		SuppressOutputScope();
		~SuppressOutputScope();

		SuppressOutputScope(const SuppressOutputScope&) = delete;
		SuppressOutputScope& operator=(const SuppressOutputScope&) = delete;
	};

	[[nodiscard]] bool IsOutputSuppressed();
}
