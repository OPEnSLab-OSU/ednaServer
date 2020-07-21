#include <StateControllers/MainStateController.hpp>
#include <Application/Application.hpp>

void Main::Idle::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	println(app.scheduleNextActiveTask().description());
};

void Main::Stop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);
	app.pump.off();
	app.shift.writeAllRegistersLow();
	app.intake.off();

	app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
	app.vm.writeToDirectory();

	auto currentTaskId = app.currentTaskId;
	app.tm.advanceTask(currentTaskId);
	app.tm.writeToDirectory();

	app.currentTaskId		= 0;
	app.status.currentValve = -1;
	sm.next();
}
