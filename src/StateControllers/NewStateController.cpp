#include <StateControllers/NewStateController.hpp>
#include <States/Stop.hpp>
#include <States/Flush.hpp>
#include <States/OffshootClean.hpp>
#include <States/Sample.hpp>
#include <States/Dry.hpp>
#include <States/Preserve.hpp>
#include <States/AirFlush.hpp>

// Sharing custom state in main
#include <StateControllers/MainStateController.hpp>

void NewStateController::setup() {
	using namespace New;

	// Alternative with default state transition:
	// registerState(Flush(), FLUSH1, OFFSHOOT_CLEAN_1);
	registerState(Flush(), FLUSH1, [this](int code) {
		switch (code) {
		case 0:
			return transitionTo(OFFSHOOT_CLEAN_1);
		default:
			halt(TRACE, "Unhandled state transition");
		}
	});

	registerState(OffshootClean(5), OFFSHOOT_CLEAN_1, FLUSH2);
	registerState(Flush(), FLUSH2, SAMPLE);
	registerState(Sample(), SAMPLE, OFFSHOOT_CLEAN_2);
	registerState(OffshootClean(10), OFFSHOOT_CLEAN_2, DRY);
	registerState(Dry(), DRY, PRESERVE);
	registerState(Preserve(), PRESERVE, AIR_FLUSH);
	registerState(AirFlush(), AIR_FLUSH, STOP);
	registerState(Main::Stop(), STOP, IDLE);
	registerState(Main::Idle(), IDLE);
};

void NewStateController::transferTaskDataToStateParameters(Task & task) {
	using namespace New;
	auto & flush1 = *getState<Flush>(FLUSH1);
	flush1.time	  = task.flushTime;

	auto & flush2 = *getState<Flush>(FLUSH2);
	flush2.time	  = task.flushTime;

	auto & sample	= *getState<Sample>(SAMPLE);
	sample.time		= task.sampleTime;
	sample.pressure = task.samplePressure;
	sample.volume	= task.sampleVolume;

	auto & dry = *getState<Dry>(DRY);
	dry.time   = task.dryTime;

	auto & preserve = *getState<Preserve>(PRESERVE);
	preserve.time	= task.preserveTime;
	println("Transferring data to states");
}