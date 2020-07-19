#pragma once
#include <KPFoundation.hpp>
#include <KPStateMachine.hpp>
#include <KPState.hpp>

#define STATE(x) constexpr const char * x = #x "_STATE"

class StateController : public KPStateMachine {
public:
	const char * beginState = nullptr;
	const char * stopState	= nullptr;
	const char * idleState	= nullptr;

	StateController(const char * name, const char * begin, const char * stop, const char * idle)
		: KPStateMachine(name),
		  beginState(begin),
		  stopState(stop),
		  idleState(idle) {}

	void begin() {
		transitionTo(beginState);
	}

	void stop() {
		transitionTo(stopState);
	}

	void idle() {
		transitionTo(idleState);
	}

	void setup() override {
		println("StateMachine Setup ");
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Decouple state machine from task object. This is where task data gets
	 *  transfered to states' parameters
	 *
	 *  @param task Task object containing states' parameters
	 *  ──────────────────────────────────────────────────────────────────────────── */
	// virtual void transferTaskDataToStateParameters(const Task & task) {}
};