#include <States/OffshootClean.hpp>
#include <Application/Application.hpp>

void OffshootClean::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();	 // Reset shift registers
	app.intake.on();
	app.shift.setPin(app.currentValveIdToPin(), HIGH);
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on(Direction::reverse);

	setTimeCondition(time, [&]() { sm.next(); });
};