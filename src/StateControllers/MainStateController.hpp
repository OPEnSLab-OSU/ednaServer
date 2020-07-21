#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>

namespace Main {
	STATE(STOP);
	STATE(IDLE);
	STATE(PRESERVE);
	STATE(DRY);
	STATE(SAMPLE);
	STATE(FLUSH);

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief This state checks if there is an active task and schedule it if any.
	 *  [Connections: 0]
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Idle : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Turn everything off and make sure that anything changes to the valve manager and task
	 *  manager are committed to persistent store.
	 *	[Connections: 1]
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Stop : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};
};	// namespace Main

class MainStateController : public StateController {
public:
	MainStateController()
		: StateController("main-state-machine", Main::FLUSH, Main::STOP, Main::IDLE) {}

	// FLUSH -> SAMPLE -> DRY -> PRESERVE -> STOP -> IDLE
	void setup() {
		using namespace Main;
		registerState(SharedStates::Flush(), FLUSH, SAMPLE);
		registerState(SharedStates::Sample(), SAMPLE, DRY);
		registerState(SharedStates::Dry(), DRY, PRESERVE);
		registerState(SharedStates::Preserve(), PRESERVE, STOP);
		registerState(Stop(), STOP, IDLE);
		registerState(Idle(), IDLE);
	}
};