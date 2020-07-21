#pragma once
#include <Components/StateController.hpp>
#include <Task/Task.hpp>
#include <States/Shared.hpp>
#include <StateControllers/MainStateController.hpp>

namespace New {
	STATE(IDLE);
	STATE(STOP);
	STATE(FLUSH1);
	STATE(FLUSH2);
	STATE(OFFSHOOT_CLEAN_1);
	STATE(OFFSHOOT_CLEAN_2);
	STATE(SAMPLE);
	STATE(DRY);
	STATE(PRESERVE);
	STATE(AIR_FLUSH);
};	// namespace New

class NewStateController : public StateController {
public:
	NewStateController()
		: StateController("new-state-controller", New::FLUSH1, New::STOP, New::IDLE) {}

	void setup() {
		using namespace New;

		// Alternative with default state transition:
		// registerState(Flush(), FLUSH1, OFFSHOOT_CLEAN_1);
		registerState(SharedStates::Flush(), FLUSH1, [this](int code) {
			switch (code) {
			case 0:
				return transitionTo(OFFSHOOT_CLEAN_1);
			default:
				halt(TRACE, "Unhandled state transition");
			}
		});

		registerState(SharedStates::OffshootClean(5), OFFSHOOT_CLEAN_1, FLUSH2);
		registerState(SharedStates::Flush(), FLUSH2, SAMPLE);
		registerState(SharedStates::Sample(), SAMPLE, OFFSHOOT_CLEAN_2);
		registerState(SharedStates::OffshootClean(10), OFFSHOOT_CLEAN_2, DRY);
		registerState(SharedStates::Dry(), DRY, PRESERVE);
		registerState(SharedStates::Preserve(), PRESERVE, AIR_FLUSH);
		registerState(SharedStates::AirFlush(), AIR_FLUSH, STOP);
		registerState(Main::Stop(), STOP, IDLE);
		registerState(Main::Idle(), IDLE);
	};

	void transferTaskDataToStateParameters(Task & task) {
		using namespace New;
		auto & flush1 = *getState<SharedStates::Flush>(FLUSH1);
		flush1.time	  = task.flushTime;

		auto & flush2 = *getState<SharedStates::Flush>(FLUSH2);
		flush2.time	  = task.flushTime;

		auto & sample	= *getState<SharedStates::Sample>(SAMPLE);
		sample.time		= task.sampleTime;
		sample.pressure = task.samplePressure;
		sample.volume	= task.sampleVolume;

		auto & dry = *getState<SharedStates::Dry>(DRY);
		dry.time   = task.dryTime;

		auto & preserve = *getState<SharedStates::Preserve>(PRESERVE);
		preserve.time	= task.preserveTime;
		println("Transferring data to states");
	}
};