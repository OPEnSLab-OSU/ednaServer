#include <States/FlushVolume.hpp>
#include <Application/Application.hpp>

void FlushVolume::enter(KPStateMachine & sm) {
	auto & app = *static_cast<Application *>(sm.controller);
	app.shift.setAllRegistersLow();
	app.intake.on();
	app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
	app.shift.write();
	app.pump.on();

	auto condition = [&]() { return app.status.waterVolume >= 500; };
	setCondition(condition, [&]() { sm.next(1); });
	setTimeCondition(time, [&]() { sm.next(); });
}