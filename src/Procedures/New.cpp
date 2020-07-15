
#include <Application/Application.hpp>

void New::Idle::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	println(app.scheduleNextActiveTask().description());
}

void New::Stop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	app.writeBallValveOff();

	app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
	app.vm.writeToDirectory();

	auto currentTaskId = app.currentTaskId;
	app.tm.advanceTask(currentTaskId);
	app.tm.writeToDirectory();

	app.currentTaskId		= 0;
	app.status.currentValve = -1;
	sm.transitionTo(StateName::IDLE);
}

void New::AirFlush::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::STOP); });
}

void New::Preserve::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::AIR_FLUSH); });
}

void New::Dry::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);

	app.shift.setAllRegistersLow();
	app.writeBallValveOff();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::PRESERVE); });
}

void New::Sample::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	app.shift.setAllRegistersLow();
	app.shift.setPin(app.currentValveIdToPin(), HIGH);	// Filter valve
	app.shift.write();
	app.pump.on();

	// This condition will be evaluated repeatedly until true then the callback will be executed
	// once
	auto const condition = [&]() {
		return timeSinceLastTransition() >= secsToMillis(time) || app.status.pressure >= pressure;
	};

	setCondition(condition, [&]() { sm.transitionTo(StateName::OFFSHOOT_CLEAN_2); });
}

void New::OffshootClean::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();	 // Reset shift registers
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on(Direction::reverse);

	setTimeCondition(time, [&]() { sm.transitionTo(nextStateName); });
};

void New::Flush::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.writeBallValveOn();
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	// To next state after 10 secs
	setTimeCondition(time, [&]() { sm.transitionTo(nextStateName); });
};