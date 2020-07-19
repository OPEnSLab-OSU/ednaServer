#include <States/Flush.hpp>
#include <Application/Application.hpp>

void Flush::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.intake.on();
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	// To next state after 10 secs
	setTimeCondition(time, [&]() { sm.next(); });
}