#include <States/AirFlush.hpp>
#include <Application/Application.hpp>

void AirFlush::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.shift.writeAllRegistersLow();
	app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	setTimeCondition(time, [&]() { sm.next(); });
}