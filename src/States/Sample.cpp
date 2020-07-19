#include <States/Sample.hpp>
#include <Application/Application.hpp>

void Sample::enter(KPStateMachine & sm) {
	// We set the latch valve to intake mode, turn on the filter valve, then the pump
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.intake.on();
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.write();
	app.pump.on();

	// This condition will be evaluated repeatedly until true then the callback will be executed
	// once
	auto const condition = [&]() {
		return timeSinceLastTransition() >= secsToMillis(time) || app.status.pressure >= pressure;
	};

	setCondition(condition, [&]() { sm.next(); });
}