#pragma once
#include <KPState.hpp>

class Sample : public KPState {
public:
	unsigned long time = 150;
	float pressure	   = 8;
	float volume	   = 1000;
	void enter(KPStateMachine & sm) override;
};