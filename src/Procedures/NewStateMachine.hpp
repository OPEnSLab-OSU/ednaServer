#include <Components/StateMachine.hpp>
#include <Procedures/New.hpp>

class NewStateMachine : public StateMachine {
public:
	NewStateMachine()
		: StateMachine("new-state-machine", New::StateName::FLUSH1, New::StateName::STOP,
					   New::StateName::IDLE) {}

	void setup() override {
		using namespace New;
		registerState(Idle(), StateName::IDLE);
		registerState(Stop(), StateName::STOP);

		registerState(Flush(StateName::OFFSHOOT_CLEAN_1), StateName::FLUSH1);
		registerState(OffshootClean(StateName::FLUSH2), StateName::OFFSHOOT_CLEAN_1);
		registerState(Flush(StateName::SAMPLE), StateName::FLUSH2);
		registerState(Sample(), StateName::SAMPLE);
		registerState(OffshootClean(StateName::DRY), StateName::OFFSHOOT_CLEAN_2);
		registerState(Dry(), StateName::DRY);
		registerState(Preserve(), StateName::PRESERVE);
		registerState(AirFlush(), StateName::AIR_FLUSH);
	}

	virtual void transferTaskDataToStateParameters(const Task & task) {
		// using namespace Ball;
		// auto & flush = *getState<StateFlush>(StateName::FLUSH);
		// flush.time	 = task.flushTime;
		// flush.volume = task.flushVolume;

		// auto & sample	= *getState<StateSample>(StateName::SAMPLE);
		// sample.time		= task.sampleTime;
		// sample.pressure = task.samplePressure;
		// sample.volume	= task.sampleVolume;

		// auto & dry = *getState<StateDry>(StateName::DRY);
		// dry.time   = task.dryTime;

		// auto & preserve = *getState<StatePreserve>(StateName::PRESERVE);
		// preserve.time	= task.preserveTime;
		// println("Transferring data to states");
	}
};