
#include <Application/Application.hpp>
#include <array>

void HyperFlush::Idle::enter(KPStateMachine & sm) {}
void HyperFlush::Stop::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.writeBallValveOff();
	app.shift.write();
	app.pump.off();

	sm.transitionTo(StateName::IDLE);
}

void HyperFlush::OffshootPreload::enter(KPStateMachine & sm) {
	// Intake valve is opened and the motor is runnning ...
	// Turnoff only the flush valve
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, LOW);

	// Reserving space ahead of time for performance
	reserve(app.vm.numberOfValvesInUse + 1);
	println("Begin preloading procedure for ", app.vm.numberOfValvesInUse, " valves...");

	int counter		 = 0;
	int prevValvePin = 0;
	for (auto valve : app.vm.valves) {
		if (valve.status == ValveStatus::unavailable) {
			continue;
		}

		// Skip the first register
		auto valvePin = valve.id + app.shift.capacityPerRegister;
		setTimeCondition(counter * preloadTime, [&app, prevValvePin, valvePin] {
			if (prevValvePin) {
				// Turn off the previuos valve
				app.shift.setPin(prevValvePin, LOW);
				println("done");
			}

			app.shift.setPin(valvePin, HIGH);
			app.shift.write();

			auto rp = app.shift.toRegisterAndPinIndices(valvePin);
			print("Preloading ", rp, "...");
		});

		prevValvePin = valvePin;
		counter++;
	}

	// Transition to the next state after the last valve
	setTimeCondition(counter * preloadTime, [&]() {
		println("done");
		sm.transitionTo(StateName::STOP);
	});
};

void HyperFlush::Flush::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.writeBallValveOn();
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	// To next state after 10 secs
	setTimeCondition(10, [&]() { sm.transitionTo(StateName::OFFSHOOT_PRELOAD); });
};