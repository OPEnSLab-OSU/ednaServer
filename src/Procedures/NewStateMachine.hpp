#include <Components/StateMachine.hpp>
#include <Procedures/New.hpp>

class NewStateMachine : public StateMachine {
public:
	NewStateMachine()
		: StateMachine(New::StateName::FLUSH1, New::StateName::STOP, New::StateName::IDLE) {}

	void setup() override {
		using namespace New;
		registerState(Idle(), StateName::IDLE);
		registerState(Stop(), StateName::STOP);
		registerState(Flush(StateName::OFFSHOOT_PRELOAD), StateName::FLUSH1);
		registerState(OffshootPreload(), StateName::OFFSHOOT_PRELOAD);
		registerState(Flush(StateName::OFFSHOOT_CLEAN), StateName::FLUSH2);
		registerState(OffshootClean(), StateName::OFFSHOOT_CLEAN);
		registerState(Flush(StateName::SAMPLE), StateName::FLUSH3);
		registerState(Sample(), StateName::SAMPLE);
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