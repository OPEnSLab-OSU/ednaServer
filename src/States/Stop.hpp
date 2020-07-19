#pragma once
#include <KPState.hpp>

class Stop : public KPState {
public:
	void enter(KPStateMachine & sm) override;
};