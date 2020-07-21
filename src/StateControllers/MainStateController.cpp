#include <StateControllers/MainStateController.hpp>
#include <Application/Application.hpp>
#include <States/Shared.hpp>

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

void MainStateController::setup() {
	using namespace Main;

	// FLUSH -> SAMPLE -> DRY -> PRESERVE -> STOP -> IDLE
	registerState(SharedStates::Flush(), FLUSH, SAMPLE);
	registerState(SharedStates::Sample(), SAMPLE, DRY);
	registerState(SharedStates::Dry(), DRY, PRESERVE);
	registerState(SharedStates::Preserve(), PRESERVE, STOP);
	registerState(Stop(), STOP, IDLE);
	registerState(Idle(), IDLE);
}