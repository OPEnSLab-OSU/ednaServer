#pragma once
#include <KPState.hpp>

namespace SharedStates {
	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Idle : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Stop : public KPState {
	public:
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Flush : public KPState {
	public:
		unsigned long time = 10;
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class FlushVolume : public KPState {
	public:
		unsigned long time	 = 10;
		unsigned long volume = 1000;
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class AirFlush : public KPState {
	public:
		unsigned long time = 15;
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Sample : public KPState {
	public:
		unsigned long time = 150;
		float pressure	   = 8;
		float volume	   = 1000;
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Dry : public KPState {
	public:
		unsigned long time = 10;
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class OffshootClean : public KPState {
	public:
		unsigned long time = 5;
		OffshootClean(unsigned long time) : time(time) {}
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class OffshootPreload : public KPState {
	public:
		int preloadTime = 5;
		void enter(KPStateMachine & sm) override;
	};

	/** ────────────────────────────────────────────────────────────────────────────
	 *
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	class Preserve : public KPState {
	public:
		unsigned long time = 0;
		void enter(KPStateMachine & sm) override;
	};
}  // namespace SharedStates
