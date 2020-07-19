#pragma once
#include <KPState.hpp>

/** ────────────────────────────────────────────────────────────────────────────
 *  @brief Flush with volume container
 *
 *  ──────────────────────────────────────────────────────────────────────────── */
class FlushVolume : public KPState {
public:
	unsigned long time	 = 10;
	unsigned long volume = 1000;
	void enter(KPStateMachine & sm) override;
};
