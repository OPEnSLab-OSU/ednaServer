#pragma once
#include <Components/StateController.hpp>
#include <Task/Task.hpp>

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

	void setup() override;
	void transferTaskDataToStateParameters(Task & task);
};