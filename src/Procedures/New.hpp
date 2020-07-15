
#pragma once
#include <KPState.hpp>

namespace New {
	namespace StateName {
		constexpr const char * IDLE				= "idle-state";
		constexpr const char * STOP				= "stop-state";
		constexpr const char * OFFSHOOT_CLEAN_1 = "offshoot-clean-1-state";
		constexpr const char * OFFSHOOT_CLEAN_2 = "offshoot-clean-2-state";
		constexpr const char * PRESERVE			= "preserve-state";
		constexpr const char * DRY				= "dry-state";
		constexpr const char * AIR_FLUSH		= "airflush-state";
		constexpr const char * SAMPLE			= "sample-state";
		constexpr const char * FLUSH2			= "flush-state-2-state";
		constexpr const char * FLUSH1			= "flush-state-1-state";
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
		unsigned long time = 150;
		float pressure	   = 8;
		float volume	   = 1000;
		void enter(KPStateMachine & sm) override;
	};

	class Dry : public KPState {
	public:
		unsigned long time = 10;
		void enter(KPStateMachine & sm) override;
	};

	class AirFlush : public KPState {
	public:
		unsigned long time = 15;
		void enter(KPStateMachine & sm) override;
	};

	class Preserve : public KPState {
	public:
		unsigned long time = 0;
		void enter(KPStateMachine & sm) override;
	};

	class OffshootClean : public KPState {
	public:
		const char * nextStateName = nullptr;
		unsigned long time		   = 5;
		OffshootClean(const char * name, int time) : nextStateName(name), time(time) {}
		void enter(KPStateMachine & sm) override;
	};

	class Flush : public KPState {
	public:
		const char * nextStateName = nullptr;
		unsigned long time		   = 10;
		Flush(const char * name) : nextStateName(name) {}
		void enter(KPStateMachine & sm) override;
	};
}  // namespace New