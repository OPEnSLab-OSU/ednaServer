#include <Components/StateMachine.hpp>
#include <Procedures/Preload.hpp>

class PreloadStateMachine : public StateMachine {
public:
	PreloadStateMachine()
		: StateMachine("preload-state-machine", Preload::StateName::FLUSH, Preload::StateName::STOP,
					   Preload::StateName::IDLE) {}

	void setup() override {
		using namespace Preload;
		registerState(Idle(), StateName::IDLE);
		registerState(Stop(), StateName::STOP);
		registerState(Flush(), StateName::FLUSH);
		registerState(OffshootPreload(), StateName::OFFSHOOT_PRELOAD);
	}

	virtual void transferTaskDataToStateParameters(const Task & task) {}
};