#pragma once

namespace LocalCommands
{
	enum class LocalCommandReadinessTransition
	{
		kNone,
		kBecameReady
	};

	class LocalCommandReadinessState
	{
	public:
		[[nodiscard]] LocalCommandReadinessTransition ObserveDataLoaded() noexcept
		{
			if (_ready) {
				return LocalCommandReadinessTransition::kNone;
			}

			_ready = true;
			return LocalCommandReadinessTransition::kBecameReady;
		}

		[[nodiscard]] bool IsReady() const noexcept
		{
			return _ready;
		}

	private:
		bool _ready = false;
	};
}
