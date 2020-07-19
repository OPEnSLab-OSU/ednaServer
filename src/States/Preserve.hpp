#pragma once
#include <KPState.hpp>

class Preserve : public KPState {
public:
	unsigned long time = 0;
	void enter(KPStateMachine & sm) override;
};