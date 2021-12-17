#include <States/Shared.hpp>
#include <Application/App.hpp>

namespace SharedStates {
    void Idle::enter(KPStateMachine & sm) {}

    void Stop::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.pump.off();
        app.shift.writeAllRegistersLow();
        app.intake.off();
        sm.next();
    }

    void Flush::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        delay(5000);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();

        // To next state after 10 secs
        setRelativeTimeCondition(time, [&]() { sm.next(); });
    }

    void Flush::update(KPStateMachine & sm) {
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
    }

    void Flush::leave(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.pump.off();
        delay(1000);
    }

    void FlushVolume::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        delay(5000);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();

        auto condition = [&]() { return app.status.waterVolume >= 500; };
        setCondition(condition, [&]() { sm.next(1); });
        setRelativeTimeCondition(time, [&]() { sm.next(); });
    }

    void FlushVolume::update(KPStateMachine & sm) {
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
    }

    void AirFlush::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.writeAllRegistersLow();
        app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();

        setTimeCondition(time, [&]() { sm.next(); });
    }

    void AirFlush::update(KPStateMachine & sm) {
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
    }

    void Sample::enter(KPStateMachine & sm) {
        // We set the latch valve to intake mode, turn on the filter valve, then the pump
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        delay(5000);
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
        app.pump.on();

        app.sensors.flow.resetVolume();
        app.sensors.flow.startMeasurement();

        app.status.maxPressure = 0;
        this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.sensors.flow.volume >= volume) {
                this->condition = "volume";
            }

            if (app.status.pressure >= pressure) {
                this->condition = "pressure";
            }

            if (timeSinceLastTransition() - 5 >= secsToMillis(time)) { //minus 5 to account for delay
                this->condition = "time";
            }

            return this->condition != nullptr;
        };

        setCondition(condition, [&]() { sm.next(); });
    }

    void Sample::update(KPStateMachine & sm){
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
    }

    void Dry::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.off();
        delay(5000);
        app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
        app.pump.on();

        setRelativeTimeCondition(time, [&]() { sm.next(); });
    }

    void Dry::update(KPStateMachine & sm) {
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
    }

    void OffshootClean::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();  // Reset shift registers
        app.intake.on();
        delay(5000);        // Delay to ensure ball intake is set properly
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on(Direction::reverse);

        setRelativeTimeCondition(time, [&]() { sm.next(); });
    };

    void OffshootClean::update(KPStateMachine & sm){
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
    }

    void OffshootClean::leave(KPStateMachine & sm){
        auto & app = *static_cast<App *>(sm.controller);
        app.pump.off();
        delay(1000);
    }

    void OffshootPreload::enter(KPStateMachine & sm) {
        // Intake valve is opened and the motor is runnning ...
        // Turnoff only the flush valve
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, LOW);
        app.shift.write();
        app.intake.on();
        delay(5000);
        app.pump.on();

        // Reserving space ahead of time for performance
        reserve(app.vm.numberOfValvesInUse + 1);
        println("Begin preloading procedure for ", app.vm.numberOfValvesInUse, " valves...");

        int counter      = 0;
        int prevValvePin = 0;
        for (const decltype(auto) valve : app.vm.valves) {
            if (valve.status == ValveStatus::unavailable) {
                continue;
            }

            // Skip the first register
            auto valvePin = valve.id + app.shift.capacityPerRegister;
            setRelativeTimeCondition(counter * preloadTime + counter, [&app, prevValvePin, valvePin]() {
                if (prevValvePin) {
                    // Turn off the previous valve
                    app.shift.setPin(prevValvePin, LOW);
                    app.pump.off();
                    app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
                    app.shift.write();
                    delay(500);
                    println("done");
                }

                app.shift.setPin(valvePin, HIGH);
                app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
                app.shift.write();
                delay(500);
                app.pump.on();
                print("Flushing offshoot ", valvePin - app.shift.capacityPerRegister, "...");
            });

            prevValvePin = valvePin;
            counter++;
        }

        // Transition to the next state after the last valve
        setRelativeTimeCondition(counter * preloadTime + counter, [&]() {
            println("done");
            sm.next();
        });
    };

    void Preserve::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.writeAllRegistersLow();
        app.intake.off();
        delay(5000);
        app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
        app.pump.on();

        setRelativeTimeCondition(time, [&]() { sm.next(); });
    }


    void Preserve::update(KPStateMachine & sm){
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
    }

    void AlcoholPurge::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.writeAllRegistersLow();
        app.intake.off();
        delay(5000);
        app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();

        setRelativeTimeCondition(time, [&]() { sm.next(); });
    }

    void AlcoholPurge::update(KPStateMachine & sm){
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
    }
}  // namespace SharedStates