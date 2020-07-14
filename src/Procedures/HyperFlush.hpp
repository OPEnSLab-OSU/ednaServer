
#pragma once
#include <KPState.hpp>

namespace HyperFlush {
	namespace StateName {
		constexpr const char * IDLE				= "idle-state";
		constexpr const char * STOP				= "stop-state";
		constexpr const char * OFFSHOOT_PRELOAD = "offshoot-preload-state";
		constexpr const char * FLUSH			= "flush-state-preload";
	};	// namespace StateName

	class Stop : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class Idle : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class OffshootPreload : public KPState {
	public:
		int preloadTime = 5;
		void enter(KPStateMachine & sm) override;
	};

	class Flush : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};
}  // namespace HyperFlush