#include <States/Preserve.hpp>
#include <Application/Application.hpp>

void Preserve::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.shift.writeAllRegistersLow();
	app.intake.off();
	app.shift.setPin(TPICDevices::ALCHOHOL_VALVE, HIGH);
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.next(); });
}