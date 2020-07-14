#include <Components/StateMachine.hpp>
#include <Procedures/HyperFlush.hpp>

class HyperFlushStateMachine : public StateMachine {
public:
	HyperFlushStateMachine()
		: StateMachine("preload-state-machine", HyperFlush::StateName::FLUSH,
					   HyperFlush::StateName::STOP, HyperFlush::StateName::IDLE) {}

	void setup() override {
		using namespace HyperFlush;
		registerState(Idle(), StateName::IDLE);
		registerState(Stop(), StateName::STOP);
		registerState(Flush(), StateName::FLUSH);
		registerState(OffshootPreload(), StateName::OFFSHOOT_PRELOAD);
	}

	virtual void transferTaskDataToStateParameters(const Task & task) {}
};