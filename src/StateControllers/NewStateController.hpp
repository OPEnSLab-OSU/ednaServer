#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>
#include <StateControllers/MainStateController.hpp>

namespace New {
    STATE(IDLE);
    STATE(STOP);
    STATE(FLUSH1);
    STATE(FLUSH2);
    STATE(OFFSHOOT_CLEAN_1);
    STATE(OFFSHOOT_CLEAN_2);
    STATE(SAMPLE);
    STATE(DRY);
    STATE(PRESERVE);
    STATE(AIR_FLUSH);

    struct Config {
        decltype(SharedStates::Flush::time) flushTime;
        decltype(SharedStates::Sample::time) sampleTime;
        decltype(SharedStates::Sample::pressure) samplePressure;
        decltype(SharedStates::Sample::volume) sampleVolume;
        decltype(SharedStates::Dry::time) dryTime;
        decltype(SharedStates::Preserve::time) preserveTime;
    };

    class Controller : public StateController, public StateControllerConfig<Config> {
    public:
        Controller() : StateController("new-state-controller") {}

        void setup() {
            // registerState(SharedStates::Flush(), FLUSH1, [this](int code) {
            // 	switch (code) {
            // 	case 0:
            // 		return transitionTo(OFFSHOOT_CLEAN_1);
            // 	default:
            // 		halt(TRACE, "Unhandled state transition");
            // 	}
            // });
            // ..or alternatively if state only has one input and one output
            registerState(SharedStates::Flush(), FLUSH1, OFFSHOOT_CLEAN_1);
            registerState(SharedStates::OffshootClean(5), OFFSHOOT_CLEAN_1, FLUSH2);
            registerState(SharedStates::Flush(), FLUSH2, SAMPLE);
            registerState(SharedStates::Sample(), SAMPLE, OFFSHOOT_CLEAN_2);
            registerState(SharedStates::OffshootClean(10), OFFSHOOT_CLEAN_2, DRY);
            registerState(SharedStates::Dry(), DRY, PRESERVE);
            registerState(SharedStates::Preserve(), PRESERVE, AIR_FLUSH);
            registerState(SharedStates::AirFlush(), AIR_FLUSH, STOP);

            // Reusing STOP and IDLE states from MainStateController
            registerState(Main::Stop(), STOP, IDLE);
            registerState(Main::Idle(), IDLE);
        };

        void configureStates() {
            decltype(auto) flush1 = getState<SharedStates::Flush>(FLUSH1);
            flush1.time           = config.flushTime;

            decltype(auto) flush2 = getState<SharedStates::Flush>(FLUSH2);
            flush2.time           = config.flushTime;

            decltype(auto) sample = getState<SharedStates::Sample>(SAMPLE);
            sample.time           = config.sampleTime;
            sample.pressure       = config.samplePressure;
            sample.volume         = config.sampleVolume;

            decltype(auto) dry = getState<SharedStates::Dry>(DRY);
            dry.time           = config.dryTime;

            decltype(auto) preserve = getState<SharedStates::Preserve>(PRESERVE);
            preserve.time           = config.preserveTime;
        }

        void begin() override {
            configureStates();
            transitionTo(FLUSH1);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }

        bool isStop() {
            return getCurrentState()->getName() == STOP;
        }
    };
};  // namespace New

using NewStateController = New::Controller;
