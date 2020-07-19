#pragma once
#include <KPState.hpp>

class OffshootClean : public KPState {
public:
	unsigned long time = 5;
	OffshootClean(unsigned long time) : time(time) {}
	void enter(KPStateMachine & sm) override;
};