#pragma once
#include <KPState.hpp>

class Idle : public KPState {
public:
	void enter(KPStateMachine & sm) override;
};
