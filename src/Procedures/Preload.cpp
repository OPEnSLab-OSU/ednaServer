
#include <Application/Application.hpp>

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

	// Reserving space for 25 state conditions for performance
	reserve(app.config.numberOfValves + 1);

	for (int i = 0; i < app.config.numberOfValves; i++) {
		setTimeCondition(i * preloadTime, [&app, i]() {
			if (i > 0) {  // Turn off the previous valve
				app.shift.setRegister(app.shift.toRegisterIndex(i - 1) + 1,
									  app.shift.toPinIndex(i - 1), LOW);
				println("------ Turning valve off (", i - 1, ")");
			}

			Serial.printf("%6lu Turning valve on  (%d)\n", millis(), i);
			app.shift.setRegister(app.shift.toRegisterIndex(i) + 1, app.shift.toPinIndex(i), HIGH);
			app.shift.write();
		});
	};

	// Transition to the next state after the last valve
	setTimeCondition(app.config.numberOfValves * preloadTime,
					 [&]() { sm.transitionTo(StateName::STOP); });
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