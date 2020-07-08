#include <Components/StateMachine.hpp>
#include <Procedures/Ball.hpp>

class BallStateMachine : public StateMachine {
public:
	BallStateMachine()
		: StateMachine(Ball::StateName::FLUSH, Ball::StateName::STOP, Ball::StateName::IDLE) {}

	void setup() override {
		using namespace Ball;
		registerState(StateIdle(), StateName::IDLE);
		registerState(StateStop(), StateName::STOP);
		registerState(StateFlush(), StateName::FLUSH);
		registerState(StateSample(), StateName::SAMPLE);
		registerState(StateDry(), StateName::DRY);
		registerState(StatePreserve(), StateName::PRESERVE);
	}

	virtual void transferTaskDataToStateParameters(const Task & task) {
		using namespace Ball;
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