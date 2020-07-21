#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>

namespace HyperFlush {
	STATE(IDLE);
	STATE(STOP);
	STATE(FLUSH);
	STATE(OFFSHOOT_PRELOAD);
}  // namespace HyperFlush

class HyperFlushStateController : public StateController {
public:
	HyperFlushStateController()
		: StateController("hyperflush-state-machine", HyperFlush::FLUSH, HyperFlush::STOP,
						  HyperFlush::IDLE) {}

	void setup() {
		using namespace HyperFlush;
		registerState(SharedStates::Flush(), FLUSH, OFFSHOOT_PRELOAD);
		registerState(SharedStates::OffshootPreload(), OFFSHOOT_PRELOAD, STOP);
		registerState(SharedStates::Stop(), STOP, IDLE);
		registerState(SharedStates::Idle(), IDLE);
	}

	template <typename T>
	void config(T && data) {
		using namespace HyperFlush;
		const auto flush = getState<SharedStates::Flush>(FLUSH);
		flush->time		 = data.flushTime;

		const auto preload	 = getState<SharedStates::OffshootPreload>(OFFSHOOT_PRELOAD);
		preload->preloadTime = data.preloadTime;
	}
};