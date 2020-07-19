#pragma once
#include <KPState.hpp>

class OffshootPreload : public KPState {
public:
	int preloadTime = 5;
	void enter(KPStateMachine & sm) override;
};