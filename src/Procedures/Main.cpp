#include <Application/Application.hpp>
#include <Procedures/Main.hpp>

namespace {
	void writeLatch(bool controlPin, ShiftRegister & shift) {
		shift.setPin(controlPin, HIGH);
		shift.write();
		delay(80);
		shift.setPin(controlPin, LOW);
		shift.write();
	}

	void writeLatchIn(ShiftRegister & shift) {
		writeLatch(0, shift);
	}

	void writeLatchOut(ShiftRegister & shift) {
		writeLatch(1, shift);
	}
}  // namespace

void Main::StateIdle::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	println(app.scheduleNextActiveTask().description());
}

void Main::StateStop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	writeLatchOut(app.shift);

	app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
	app.vm.writeToDirectory();

	auto currentTaskId = app.currentTaskId;
	app.tm.advanceTask(currentTaskId);
	app.tm.writeToDirectory();

	app.currentTaskId		= 0;
	app.status.currentValve = -1;
	sm.transitionTo(StateName::IDLE);
}

void Main::StatePreserve::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	writeLatchOut(app.shift);
	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::STOP); });
}

void Main::StateDry::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	app.shift.setAllRegistersLow();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::PRESERVE); });
}

void Main::StateSample::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	ValveBlock vBlock = app.currentValveNumberToBlock();

	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	app.shift.setAllRegistersLow();
	writeLatchIn(app.shift);
	app.shift.setRegister(vBlock.regIndex, vBlock.pinIndex, HIGH);	// Filter valve
	app.shift.write();
	app.pump.on();

	// This condition will be evaluated repeatedly until true then the callback will be executed
	// once
	auto const condition = [&]() {
		return timeSinceLastTransition() >= secsToMillis(time) || app.status.pressure >= pressure;
	};

	setCondition(condition, [&]() {
		writeLatchOut(app.shift);
		sm.transitionTo(StateName::DRY);
	});
}

void Main::StateFlush::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	time			  = app.tm.tasks[app.currentTaskId].flushTime;

	app.shift.setAllRegistersLow();
	writeLatchIn(app.shift);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.transitionTo(StateName::SAMPLE); });
}