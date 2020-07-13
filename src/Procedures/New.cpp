
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

void New::AirFlush::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(15000, [&]() { sm.transitionTo(StateName::STOP); });
}

void New::Preserve::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::AIR_FLUSH); });
}

void New::Dry::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.setAllRegistersLow();
	writeBallValveOff(app.shift);
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(10, [&]() { sm.transitionTo(StateName::PRESERVE); });
}

void New::Sample::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	app.shift.setAllRegistersLow();
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);	// Filter valve
	app.shift.write();
	app.pump.on();

	// This condition will be evaluated repeatedly until true then the callback will be executed
	// once
	auto const condition = [&]() {
		return timeSinceLastTransition() >= secsToMillis(time) || app.status.pressure >= pressure;
	};

	setCondition(condition, [&]() {
		// writeLatchOut(app.shift);
		sm.transitionTo(StateName::OFFSHOOT_CLEAN_2);
	});
}

void New::OffshootClean::enter(KPStateMachine & sm) {
	auto & app				= *static_cast<Application *>(sm.controller);
	const int valveId		= app.status.currentValve;
	const int registerIndex = app.shift.toRegisterIndex(valveId) + 1;
	const int pinIndex		= app.shift.toPinIndex(valveId);

	app.shift.setAllRegistersLow();	 // Reset shift registers
	app.shift.setRegister(registerIndex, pinIndex, HIGH);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(cleanTime, [&]() { sm.transitionTo(nextStateName); });
};

void New::Flush::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	writeBallValveOn(app.shift);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	// To next state after 10 secs
	setTimeCondition(10000, [&]() { sm.transitionTo(nextStateName); });
};