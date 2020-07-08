#include <Procedures/StateMachine.hpp>
#include <Procedures/Main.hpp>

class MainStateMachine : public StateMachine {
public:
	MainStateMachine()
		: StateMachine(Main::StateName::FLUSH, Main::StateName::STOP, Main::StateName::IDLE) {}

	void setup() override {
		using namespace Main;
		registerState(StateIdle(), StateName::IDLE);
		registerState(StateStop(), StateName::STOP);
		registerState(StateFlush(), StateName::FLUSH);
		registerState(StateSample(), StateName::SAMPLE);
		registerState(StateDry(), StateName::DRY);
		registerState(StatePreserve(), StateName::PRESERVE);
	}

	virtual void transferTaskDataToStateParameters(const Task & task) {
		using namespace Main;
		auto & flush = *getState<StateFlush>(StateName::FLUSH);
		flush.time	 = task.flushTime;
		flush.volume = task.flushVolume;

		auto & sample	= *getState<StateSample>(StateName::SAMPLE);
		sample.time		= task.sampleTime;
		sample.pressure = task.samplePressure;
		sample.volume	= task.sampleVolume;

		auto & dry = *getState<StateDry>(StateName::DRY);
		dry.time   = task.dryTime;

		auto & preserve = *getState<StatePreserve>(StateName::PRESERVE);
		preserve.time	= task.preserveTime;
		println("Transferring data to states");
	}
};