#pragma once
#include <Components/StateController.hpp>

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

	void setup() override;

	template <typename T>
	void config(T && data) {}
};