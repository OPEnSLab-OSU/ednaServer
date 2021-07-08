#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>
#include <StateControllers/MainStateController.hpp>

namespace NowT{
    STATE(IDLE);
    STATE(FLUSH_1);
    STATE(OFFSHOOT_CLEAN_1);
    STATE(FLUSH_2);
    STATE(SAMPLE);
    STATE(OFFSHOOT_CLEAN_2);
    STATE(DRY);
    STATE(PRESERVE);
    STATE(AIR_FLUSH);
    STATE(STOP);

    struct Config {
        decltype(SharedStates::Flush::time) flushTime;
        decltype(SharedStates::Sample::time) sampleTime;
        decltype(SharedStates::Sample::pressure) samplePressure;
        decltype(SharedStates::Sample::volume) sampleVolume;
        decltype(SharedStates::Dry::time) dryTime;
        decltype(SharedStates::Preserve::time) preserveTime;
        decltype(SharedStates::OffshootPreload::preloadTime) preloadTime;
    };

    class Controller : public StateControllerWithConfig<Config> {
    public:
        Controller() : StateControllerWithConfig("nowtask-state-controller") {}

        void setup();

        void configureStates() {
            decltype(auto) flush1 = getState<SharedStates::Flush>(FLUSH_1);
            flush1.time           = config.flushTime;

            decltype(auto) flush2 = getState<SharedStates::Flush>(FLUSH_2);
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
            transitionTo(FLUSH_1);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }

        unsigned long get_total_time() {
            return config.flushTime + config.flushTime + config.sampleTime + config.dryTime + config.preserveTime;
        }
    };
}  // namespace HyperFlush

using NowTaskStateController = NowT::Controller;