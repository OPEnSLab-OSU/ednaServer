#include <States/Shared.hpp>
#include <Application/App.hpp>

namespace SharedStates {
    void Idle::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        println(app.scheduleNextActiveTask().description());
    }

    void Stop::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.pump.off();
        app.shift.writeAllRegistersLow();
        app.intake.off();
        app.sensors.flow.stopMeasurement();
        app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
        app.vm.writeToDirectory();

        auto currentTaskId = app.currentTaskId;
        app.tm.advanceTask(currentTaskId);
        app.tm.writeToDirectory();

        app.currentTaskId       = 0;
        app.status.currentValve = -1;
        sm.next();
    }

    void Flush::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        // 5 seconds needed to turn ball intake on
        setTimeCondition(5, [&app](){
            app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
            app.shift.write();
        });

        //wait 1 second after valve opens before during on pump
        setTimeCondition(6, [&app](){
            app.pump.on();
        });


        // To next state after 10 secs
        setTimeCondition(time + 6, [&]() { sm.next(); });
    }

    void Flush::update(KPStateMachine & sm) {
        //don't update valve status during first 5 seconds
        //because valve isn't supposed to be on
        if(timeSinceLastTransition() < 5000){
            return;
        }
        // returns if it has already updated in the last second
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
        setTimeCondition(5, [&app](){
            app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
            app.shift.write();
            app.pump.on();
        });

        setTimeCondition(6, [&app](){
            app.pump.on();
        });

        auto condition = [&]() { return app.status.waterVolume >= 500; };
        setCondition(condition, [&]() { sm.next(1); });
        setTimeCondition(time + 6, [&]() { sm.next(); });
    }

    void FlushVolume::update(KPStateMachine & sm) {
        if(timeSinceLastTransition() < 5000){
            return;
        }
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

        setTimeCondition(1, [&app](){
            app.pump.on();
        });

        setTimeCondition(time + 1, [&]() { sm.next(); });
    }

    void AirFlush::update(KPStateMachine & sm) {
        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
    }

    void Sample::enter(KPStateMachine & sm) {
        // We set the latch valve to intake mode, turn on the filter valve, then the pump
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        setTimeCondition(5, [&app](){
            app.shift.setPin(app.currentValveIdToPin(), HIGH);
            app.shift.write();
        });

        setTimeCondition(6,  [&app](){
           app.pump.on(); 
        });
        
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

            if (timeSinceLastTransition() - 6 >= secsToMillis(time)) { //minus 6 to account for delay
                this->condition = "time";
            }

            return this->condition != nullptr;
        };

        setCondition(condition, [&]() { sm.next(); });
    }

    void Sample::update(KPStateMachine & sm){
        if(timeSinceLastTransition() < 5000){
            return;
        }

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

        setTimeCondition(5, [&app](){
            app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
            app.shift.setPin(app.currentValveIdToPin(), HIGH);
            app.shift.write();
        });

        setTimeCondition(6, [&app](){
            app.pump.on();
        });

        setTimeCondition(time + 6, [&]() { sm.next(); });
    }

    void Dry::update(KPStateMachine & sm) {
        if(timeSinceLastTransition() < 5000){
            return;
        }
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
        app.pump.off();
        app.intake.on();
       // Delay to ensure ball intake is set properly
        setTimeCondition(5, [&app](){
            app.shift.setPin(app.currentValveIdToPin(), HIGH);
            app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
            app.shift.write();
        });

        setTimeCondition(6, [&app](){
            app.pump.on(Direction::reverse);
        });

        //turn off pump right before state transition (leave function would be harder to have time delay)
        setTimeCondition(time + 6, [&app](){
            app.pump.off();
        });

        setTimeCondition(time + 7, [&]() { sm.next(); });
    };

    void OffshootClean::update(KPStateMachine & sm){
        if( timeSinceLastTransition() < 5000){
            return;
        }
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

    void OffshootPreload::update(KPStateMachine & sm){
        if(timeSinceLastTransition() < 5000){
            return;
        }

        if ((unsigned long) (millis() - updateTime) < updateDelay) {
            return;
        }

        updateTime = millis();
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.write();
    }

    void OffshootPreload::enter(KPStateMachine & sm) {
        // Intake valve is opened and the motor is runnning ...
        // Turnoff only the flush valve
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.shift.write();
        app.intake.on();
        setTimeCondition(5, [&app](){
            app.pump.on();
        });
        
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
            setTimeCondition(counter * preloadTime + 5, [&app, prevValvePin, valvePin]() {
                if (prevValvePin) {
                    // Turn off the previous valve
                    app.shift.setPin(prevValvePin, LOW);
                    app.pump.off();
                    app.shift.write();
                    println("done");
                }

                app.shift.setPin(valvePin, HIGH);
                app.shift.write();
                
                print("Flushing offshoot ", valvePin - app.shift.capacityPerRegister, "...");
            });

            setTimeCondition(counter * preloadTime + 6, [&app, prevValvePin, valvePin]() {
                app.pump.on();
            });

            prevValvePin = valvePin;
            counter++;
        }

        // Transition to the next state after the last valve
        setTimeCondition(counter * preloadTime + 6, [&]() {
            println("done");
            sm.next();
        });
    };

    void Preserve::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.pump.off();
        app.shift.writeAllRegistersLow();
        app.intake.off();
        setTimeCondition(5, [&app](){
            app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
            app.shift.setPin(app.currentValveIdToPin(), HIGH);
            app.shift.write();
        });

        setTimeCondition(6, [&app](){
            app.pump.on();
        });
        setTimeCondition(time + 6, [&]() { sm.next(); });
    }


    void Preserve::update(KPStateMachine & sm){
        if(timeSinceLastTransition() < 5000){
            return;
        }
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
        app.pump.off();
        setTimeCondition(5, [&app](){
            app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
            app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
            app.shift.write();
        });
        setTimeCondition(6, [&app](){
            app.pump.on();
        });
        setTimeCondition(time + 6, [&]() { sm.next(); });
    }

    void AlcoholPurge::update(KPStateMachine & sm){
        if(timeSinceLastTransition() < 5000){
            return;
        }
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