#pragma once
#include <KPState.hpp>

namespace Main {
	namespace StateName {
		constexpr const char * IDLE		= "idle-state";
		constexpr const char * STOP		= "stop-state";
		constexpr const char * PRESERVE = "preserve-state";
		constexpr const char * DRY		= "dry-state";
		constexpr const char * SAMPLE	= "sample-state";
		constexpr const char * FLUSH	= "flush-state";
	};	// namespace StateName

	class StateIdle : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class StateStop : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	class StatePreserve : public KPState {
	public:
		unsigned long time = 0;
		float pressure	   = 8;
		float volume	   = 100;
		void enter(KPStateMachine & sm) override;
	};

	class StateDry : public KPState {
	public:
		unsigned long time = 10;
		void enter(KPStateMachine & sm) override;
	};

	class StateSample : public KPState {
	public:
		unsigned long time = 15;
		float pressure	   = 8;
		float volume	   = 1000;
		void enter(KPStateMachine & sm) override;
	};

	class StateFlush : public KPState {
	public:
		unsigned long time = 15;
		float pressure	   = 8;
		float volume	   = 1000;
		void enter(KPStateMachine & sm) override;
	};
};	// namespace Main