#include <StateControllers/HyperFlushStateController.hpp>
#include <States/Shared.hpp>

void HyperFlushStateController::setup() {
	using namespace HyperFlush;
	registerState(SharedStates::Flush(), FLUSH, OFFSHOOT_PRELOAD);
	registerState(SharedStates::OffshootPreload(), OFFSHOOT_PRELOAD, STOP);
	registerState(SharedStates::Stop(), STOP, IDLE);
	registerState(SharedStates::Idle(), IDLE);
}