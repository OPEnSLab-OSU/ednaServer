#pragma once
#include <KPState.hpp>

class AirFlush : public KPState {
public:
	unsigned long time = 15;
	void enter(KPStateMachine & sm) override;

	template <typename T>
	void config(T & data) {
		time = data.time;
	}
};