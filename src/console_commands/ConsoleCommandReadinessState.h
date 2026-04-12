#pragma once

namespace ConsoleCommands
{
	enum class ConsoleCommandReadinessTransition
	{
		kNone,
		kBecameReady,
		kBecameUnavailable
	};

	class ConsoleCommandReadinessState
	{
	public:
		[[nodiscard]] ConsoleCommandReadinessTransition Observe(const bool ready) noexcept
		{
			if (!_observed) {
				_observed = true;
				_ready = ready;
				return ready ? ConsoleCommandReadinessTransition::kBecameReady : ConsoleCommandReadinessTransition::kNone;
			}

			if (_ready == ready) {
				return ConsoleCommandReadinessTransition::kNone;
			}

			_ready = ready;
			return ready ? ConsoleCommandReadinessTransition::kBecameReady : ConsoleCommandReadinessTransition::kBecameUnavailable;
		}

		[[nodiscard]] bool IsReady() const noexcept
		{
			return _ready;
		}

	private:
		bool _observed = false;
		bool _ready = false;
	};
}
