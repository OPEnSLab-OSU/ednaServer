#include <States/Stop.hpp>
#include <Application/Application.hpp>

void Stop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	app.intake.off();
	sm.next();
}