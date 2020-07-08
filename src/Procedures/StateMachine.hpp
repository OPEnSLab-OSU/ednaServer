#pragma once
#include <KPFoundation.hpp>
#include <KPStateMachine.hpp>
#include <Task/Task.hpp>

class StateMachine : public KPStateMachine {
public:
	const char * entryStateName = nullptr;
	const char * stopStateName	= nullptr;
	const char * idleStateName	= nullptr;

	StateMachine(const char * entryStateName,
				 const char * stopStateName,
				 const char * idleStateName)
		: KPStateMachine("controlled-state-machine"),
		  entryStateName(entryStateName),
		  stopStateName(stopStateName),
		  idleStateName(idleStateName) {}

	void begin() {
		transitionTo(entryStateName);
	}

	void stop() {
		transitionTo(stopStateName);
	}

	void idle() {
		transitionTo(idleStateName);
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
	virtual void transferTaskDataToStateParameters(const Task & task) {}
};