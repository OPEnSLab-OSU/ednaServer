#include <Application/Application.hpp>
#include <Procedures/Ball.hpp>

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

void Ball::StateIdle::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	println(app.scheduleNextActiveTask().description());
}

void Ball::StateStop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	writeBallValveOff(app.shift);

	app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
	app.vm.writeToDirectory();

	auto currentTaskId = app.currentTaskId;
	app.tm.advanceTask(currentTaskId);
	app.tm.writeToDirectory();

	app.currentTaskId		= 0;
	app.status.currentValve = -1;
	sm.transitionTo(StateName::IDLE);
}

void Ball::StatePreserve::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.writeAllRegistersLow();
	writeBallValveOff(app.shift);
	app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::STOP); });
}

void Ball::StateDry::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.setAllRegistersLow();
	writeBallValveOff(app.shift);
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::PRESERVE); });
}

void Ball::StateSample::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	app.shift.setAllRegistersLow();
	writeBallValveOn(app.shift);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);	// Filter valve
	app.shift.write();
	app.pump.on();

	// This condition will be evaluated repeatedly until true then the callback will be executed
	// once
	auto const condition = [&]() {
		return timeSinceLastTransition() >= secsToMillis(time) || app.status.pressure >= pressure;
	};

	setCondition(condition, [&]() { sm.transitionTo(StateName::DRY); });
}

void Ball::StateFlush::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);

	app.shift.setAllRegistersLow();
	writeBallValveOn(app.shift);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::SAMPLE); });
}