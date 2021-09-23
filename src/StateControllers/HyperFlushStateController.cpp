#include <StateControllers/HyperFlushStateController.hpp>
#include <Application/App.hpp>
void HyperFlushStateController::setup() {
    // FLUSH -> OFFSHOOT_PRELOAD -> STOP -> IDLE
    registerState(SharedStates::Flush(), FLUSH, OFFSHOOT_PRELOAD);
    registerState(SharedStates::OffshootPreload(), OFFSHOOT_PRELOAD, [this](int code) {
        auto & app = *static_cast<App *>(controller);
        app.sensors.flow.stopMeasurement();

        switch (code) {
        case 0:
            return transitionTo(STOP);
        default:
            halt(TRACE, "Unhandled state transition: ", code);
        }
    });
    registerState(SharedStates::Stop(), STOP, IDLE);
    registerState(SharedStates::Idle(), IDLE);
};