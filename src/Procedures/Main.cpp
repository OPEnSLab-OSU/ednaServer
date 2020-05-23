#include <Application/Application.hpp>
#include <Procedures/Main.hpp>

void StateIdle::enter(KPStateMachine & sm) {}

void StateStop::enter(KPStateMachine & sm) {
	println("StateStop");

	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	app.shift.writeLatchOut();

	app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
	app.vm.writeValvesToDirectory();

	app.tm.nearestActiveTask()->nextValve();
	app.tm.cleanUpCompletedTasks();
	app.tm.writeTaskArrayToDirectory();
	app.scheduleNextActiveTask();

	sm.transitionTo(StateName::IDLE);
}

void StatePreserve::enter(KPStateMachine & sm) {
	println("StatePreserve");

	Application & app = *static_cast<Application *>(sm.controller);

	int r = app.shift.toRegisterIndex(app.status.currentValve) + 1;
	int b = app.shift.toPinIndex(app.status.currentValve);

	app.shift.writeLatchOut();
	app.shift.writeAllRegistersLow();
	app.shift.setPin(AlcoholValveBitIndex, HIGH);
	app.shift.setRegister(r, b, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() {
		sm.transitionTo(StateName::STOP);
	});
}

void StateDry::enter(KPStateMachine & sm) {
	println("StateDry");

	Application & app = *static_cast<Application *>(sm.controller);

	int r = app.shift.toRegisterIndex(app.status.currentValve) + 1;
	int b = app.shift.toPinIndex(app.status.currentValve);

	app.shift.setAllRegistersLow();
	app.shift.setPin(AirValveBitIndex, HIGH);
	app.shift.setRegister(r, b, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() {
		sm.transitionTo(StateName::PRESERVE);
	});
}

void StateSample::enter(KPStateMachine & sm) {
	println("StateSample");

	Application & app = *static_cast<Application *>(sm.controller);

	int r = app.shift.toRegisterIndex(app.status.currentValve) + 1;
	int b = app.shift.toPinIndex(app.status.currentValve);

	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	app.shift.setAllRegistersLow();
	app.shift.writeLatchIn();
	app.shift.setRegister(r, b, HIGH);	// Filter valve
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
	println("StateFlush");
	Application & app = *static_cast<Application *>(sm.controller);

	app.shift.setAllRegistersLow();
	app.shift.writeLatchIn();
	app.shift.setPin(FlushValveBitIndex, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() {
		sm.transitionTo(StateName::SAMPLE);
	});
}

// void StateDecon::pressurizeTo(float target) {
// }

// void StateDecon::enter(KPStateMachine & sm) {
// 	Application & app = *static_cast<Application *>(sm.controller);

// 	app.pump.off();
// 	app.shift.writeLatchOut();
// 	app.shift.writeAllRegistersLow();

// 	// Turn on the flush valve, the air intake, and the pump to clean out the main pipe
// 	app.shift.setPin(FlushValveBitIndex, HIGH);
// 	app.shift.setPin(AirValveBitIndex, HIGH);
// 	app.shift.write();
// 	app.pump.on();
// 	// auto const flushMainPipe =

// 	// Turn the pump off at 10th second
// 	setTimeCondition(secsToMillis(10), [&]() {
// 		app.pump.off();
// 	});

// 	// Turn the pump on to 75% power at 12th second then close the flush valve
// 	setTimeCondition(secsToMillis(12), [&]() {
// 		app.pump.pwm(0.75);
// 		app.shift.setPin(FlushValveBitIndex, LOW);
// 		app.shift.write();
// 	});

// 	auto const condition = [&]() {
// 		return timeSinceLastTransition() >= 12 && app.status.pressure >= 2;
// 	};

// 	setCondition(condition, [&]() {
// 		int r = app.shift.toRegisterIndex(app.status.currentValve) + 1;
// 		int b = app.shift.toPinIndex(app.status.currentValve);

// 		app.pump.off();
// 		app.shift.setRegister(r, b, HIGH);
// 		app.shift.write();
// 		delay(50);

// 		sm.transitionTo(StateName::STOP);
// 	});
// }