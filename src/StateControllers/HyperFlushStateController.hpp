#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>

namespace HyperFlush {
    STATE(IDLE);
    STATE(STOP);
    STATE(FLUSH);
    STATE(OFFSHOOT_PRELOAD);

    struct Config {
        decltype(SharedStates::Flush::time) flushTime;
        decltype(SharedStates::OffshootPreload::preloadTime) preloadTime;
    };

    class Controller : public StateControllerWithConfig<Config> {
    public:
        Controller() : StateControllerWithConfig("hyperflush-state-machine") {}

        void setup();


        void begin() override {
            decltype(auto) flush = getState<SharedStates::Flush>(FLUSH);
            flush.time           = config.flushTime;

            decltype(auto) preload = getState<SharedStates::OffshootPreload>(OFFSHOOT_PRELOAD);
            preload.preloadTime    = config.preloadTime;

            transitionTo(FLUSH);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }
    };
}  // namespace HyperFlush

using HyperFlushStateController = HyperFlush::Controller;
