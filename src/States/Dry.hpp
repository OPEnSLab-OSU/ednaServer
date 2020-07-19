#pragma once
#include <KPState.hpp>

class Dry : public KPState {
public:
	unsigned long time = 10;
	void enter(KPStateMachine & sm) override;
};
