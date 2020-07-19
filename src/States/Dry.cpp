#include <States/Dry.hpp>
#include <Application/Application.hpp>

void Dry::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);

	app.shift.setAllRegistersLow();
	app.intake.off();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.next(); });
}