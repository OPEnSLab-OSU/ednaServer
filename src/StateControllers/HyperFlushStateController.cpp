#include <StateControllers/HyperFlushStateController.hpp>
#include <States/Flush.hpp>
#include <States/FlushVolume.hpp>
#include <States/OffshootPreload.hpp>
#include <States/Stop.hpp>
#include <States/Idle.hpp>

void HyperFlushStateController::setup() {
	using namespace HyperFlush;
	registerState(Flush(), FLUSH, OFFSHOOT_PRELOAD);
	registerState(OffshootPreload(), OFFSHOOT_PRELOAD, STOP);
	registerState(Stop(), STOP, IDLE);
	registerState(Idle(), IDLE);
}