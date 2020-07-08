
#pragma once
#include <KPState.hpp>

namespace New {
	namespace StateName {
		constexpr const char * IDLE				= "idle-state";
		constexpr const char * STOP				= "stop-state";
		constexpr const char * PRESERVE			= "preserve-state";
		constexpr const char * OFFSHOOT_CLEAN	= "offshoot-clean-state";
		constexpr const char * OFFSHOOT_PRELOAD = "offshoot-preload-state";
		constexpr const char * SAMPLE			= "sample-state";
		constexpr const char * FLUSH3			= "flush-state-3";
		constexpr const char * FLUSH2			= "flush-state-2";
		constexpr const char * FLUSH1			= "flush-state-1";
	};	// namespace StateName

	class Stop : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class Idle : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class Sample : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class OffshootClean : public KPState {
	public:
		int cleanTime = 2000;
		void enter(KPStateMachine & sm) override;
	};

	class OffshootPreload : public KPState {
	public:
		int preloadTime = 2000;
		void enter(KPStateMachine & sm) override;
	};

	class Flush : public KPState {
	public:
		const char * nextStateName = nullptr;
		Flush(const char * name) : nextStateName(name) {}
		void enter(KPStateMachine & sm) override;
	};
}  // namespace New