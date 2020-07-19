#pragma once
#include <KPState.hpp>

/** ────────────────────────────────────────────────────────────────────────────
 *  @brief
 *  @pre
 *  @post
 *
 *  ──────────────────────────────────────────────────────────────────────────── */
class Flush : public KPState {
public:
	unsigned long time = 10;
	void enter(KPStateMachine & sm) override;
};
