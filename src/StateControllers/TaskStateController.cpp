#include <StateControllers/TaskStateController.hpp>
#include <Application/App.hpp>
void TaskStateController::setup() {
    // registerState(SharedStates::Flush(), FLUSH1, [this](int code) {
    // 	switch (code) {
    // 	case 0:
    // 		return transitionTo(OFFSHOOT_CLEAN_1);
    // 	default:
    // 		halt(TRACE, "Unhandled state transition");
    // 	}
    // });
    // ..or alternatively if state only has one input and one output

    registerState(SharedStates::Flush(), FLUSH_1, OFFSHOOT_CLEAN_1);
    registerState(SharedStates::OffshootClean(5), OFFSHOOT_CLEAN_1, FLUSH_2);
    registerState(SharedStates::Flush(), FLUSH_2, SAMPLE);
    registerState(SharedStates::Sample(), SAMPLE, [this](int code) {
        auto & app = *static_cast<App *>(controller);
        app.sensors.flow.stopMeasurement();
        app.logAfterSample();

        switch (code) {
        case 0:
            return transitionTo(OFFSHOOT_CLEAN_2);
        default:
            halt(TRACE, "Unhandled state transition: ", code);
        }
    });
    registerState(SharedStates::OffshootClean(10), OFFSHOOT_CLEAN_2, DRY);
    registerState(SharedStates::Dry(), DRY, PRESERVE);
    registerState(SharedStates::Preserve(), PRESERVE, AIR_FLUSH);
    registerState(SharedStates::AirFlush(), AIR_FLUSH, STOP);
    registerState(SharedStates::Stop(), STOP, IDLE);
    registerState(SharedStates::Idle(), IDLE);
};