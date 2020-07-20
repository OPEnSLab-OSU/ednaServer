#include <States/OffshootPreload.hpp>
#include <Application/Application.hpp>

void OffshootPreload::enter(KPStateMachine & sm) {
	// Intake valve is opened and the motor is runnning ...
	// Turnoff only the flush valve
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, LOW);
	app.intake.on();

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
				// Turn off the previous valve
				app.shift.setPin(prevValvePin, LOW);
				println("done");
			}

			app.shift.setPin(valvePin, HIGH);
			app.shift.write();
			print("Flushing offshoot ", valvePin, "...");
		});

		prevValvePin = valvePin;
		counter++;
	}

	// Transition to the next state after the last valve
	setTimeCondition(counter * preloadTime, [&]() {
		println("done");
		sm.next();
	});
};