
#include <Application/Application.hpp>
#include <array>

namespace {
	void writeBallValveOn(ShiftRegister & shift) {
		shift.setPin(1, HIGH);
		shift.setPin(0, LOW);
		shift.write();
	}

	void writeBallValveOff(ShiftRegister & shift) {
		shift.setPin(0, HIGH);
		shift.setPin(1, LOW);
		shift.write();
	}
}  // namespace

void Preload::Idle::enter(KPStateMachine & sm) {}
void Preload::Stop::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	writeBallValveOff(app.shift);
	app.shift.write();
	app.pump.off();

	sm.transitionTo(StateName::IDLE);
}

void Preload::OffshootPreload::enter(KPStateMachine & sm) {
	// Intake valve is opened and the motor is runnning ...
	// Turnoff only the flush valve
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, LOW);

	// Reserving space ahead of time for performance
	reserve(app.vm.numberOfValvesInUse + 1);
	println("Begin preloading procedure for ", app.vm.numberOfValvesInUse, " valves...");

	int counter		 = 0;
	int prevValvePin = -1;
	for (auto valve : app.vm.valves) {
		if (valve.status == ValveStatus::unavailable) {
			continue;
		}

		// Skip the first register
		auto valvePin = valve.id + app.shift.capacityPerRegister;
		setTimeCondition(counter * preloadTime, [&app, prevValvePin, valvePin] {
			if (prevValvePin != -1) {
				// Turn off the previuos valve
				app.shift.setPin(prevValvePin, LOW);
				println("done");
			}

			auto rp = app.shift.toRegisterAndPinIndices(valvePin);
			app.shift.setPin(valvePin, HIGH);
			Serial.printf("%8lu Preloading (%d,%d)...", millis(), rp.first, rp.second);
			app.shift.write();
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

void Preload::Flush::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	writeBallValveOn(app.shift);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	// To next state after 10 secs
	setTimeCondition(10, [&]() { sm.transitionTo(StateName::OFFSHOOT_PRELOAD); });
};