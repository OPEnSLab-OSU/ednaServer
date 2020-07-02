#include <Application/Application.hpp>
#include <Procedures/Main.hpp>

void StateIdle::enter(KPStateMachine & sm) {}

void StateStop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	app.shift.writeLatchOut();

	app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
	app.vm.writeToDirectory();

	auto currentTaskId = app.currentTaskId;
	app.tm.advanceTask(currentTaskId);
	app.tm.writeToDirectory();

	sm.transitionTo(StateName::IDLE);
}

void StatePreserve::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.writeLatchOut();
	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() {
		sm.transitionTo(StateName::STOP);
	});
}

void StateDry::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.setAllRegistersLow();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() {
		sm.transitionTo(StateName::PRESERVE);
	});
}

void StateSample::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	app.shift.setAllRegistersLow();
	app.shift.writeLatchIn();
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);	// Filter valve
	app.shift.write();
	app.pump.on();

	// This condition will be evaluated repeatedly until true then the callback will be executed once
	auto const condition = [&]() {
		return timeSinceLastTransition() >= secsToMillis(time) || app.status.pressure >= pressure;
	};

	setCondition(condition, [&]() {
		app.shift.writeLatchOut();
		sm.transitionTo(StateName::DRY);
	});
}

void StateFlush::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.shift.writeLatchIn();
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() {
		sm.transitionTo(StateName::SAMPLE);
	});
}