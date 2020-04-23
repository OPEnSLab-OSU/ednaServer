#pragma once

#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPSerialInput.hpp>
#include <KPServer.hpp>
#include <KPStateMachine.hpp>
#include <KPAction.hpp>

#include <Application/Config.hpp>
#include <Application/Constants.hpp>
#include <Application/States.hpp>
#include <Application/Status.hpp>

#include <Components/Pump.hpp>
#include <Components/ShiftRegister.hpp>
#include <Components/Power.hpp>

#include <Valve/Valve.hpp>
#include <Valve/ValveManager.hpp>

#include <Task/Task.hpp>
#include <Task/TaskManager.hpp>

#include <Utilities/JsonEncodableDecodable.hpp>

#define AirValveBitIndex	 2
#define AlcoholValveBitIndex 3
#define FlushValveBitIndex	 5

extern void printDirectory(File dir, int numTabs);

class Application : public KPController, KPSerialInputListener {
private:
	void setupServer();
	void commandReceived(const String & line) override;

public:
	KPServer web{"web-server", "eDNA-test", "password"};
	KPStateMachine sm{"state-machine"};
	KPFileLoader fileLoader{"file-loader", SDCard_Pin};
	KPActionScheduler<10> scheduler{"scheduler"};

	Pump pump{"pump", Motor_Forward_Pin, Motor_Reverse_Pin};
	Power power{"power"};
	ShiftRegister shift{"shift-register", 32, SR_Data_Pin, SR_Clock_Pin, SR_Latch_Pin};

	Config config{ProgramSettings::CONFIG_FILE_PATH};
	Status status;

	ValveManager vm;
	TaskManager tm;

private:
	void setup() override {
		KPController::setup();
		KPSerialInput::instance().addListener(this);

		// Setup serial communication
		Serial.begin(115200);
		while (!Serial) {
		};
		// delay(5000);

		addComponent(sm);
		sm.addListener(&status);
		sm.registerState(StateIdle(), StateName::IDLE);
		sm.registerState(StateStop(), StateName::STOP);
		sm.registerState(StateFlush(), StateName::FLUSH);
		sm.registerState(StateSample(), StateName::SAMPLE);
		sm.registerState(StatePreserve(), StateName::PRESERVE);
		sm.registerState(StateDecon(), StateName::DECON);

		addComponent(web);
		setupServer();

		addComponent(fileLoader);
		addComponent(shift);
		addComponent(power);
		addComponent(pump);
		addComponent(scheduler);

		web.begin();

		config.load();
		status.init(config);

		vm.init(config);
		vm.addListener(&status);
		vm.loadValvesFromDirectory(config.valveFolder);

		tm.init(config);
		tm.loadTasksFromDirectory(config.taskFolder);

		// Transition to idle state
		sm.transitionTo(StateName::IDLE);

		power.onInterrupt([]() {
			println("Alarm Triggered");
		});

		auto load = [this]() { scheduleNextTask(); };
		run(1000, load, scheduler);
	}

	void handleTask(Task & task) {
		// NOTE: Waking up after the task schdule
		time_t timenow = now();
		if (timenow >= task.schedule) {
			// vm.setValveStatu
		}

		// NOTE: Waking up 10 secs before schedule time
		if (timenow < task.schedule && timenow >= task.schedule - 10) {
		}

		// NOTE: Waking up more than 10 secs before the schedule time
		if (timenow < task.schedule - 10) {
			if (status.preventShutdown) {
				return;
			}
		}
	}

	bool scheduleNextTask() {
		Task * task_ptr = tm.next();
		if (task_ptr == nullptr) {
			println("No task available");
			return false;
		}

		Task & task		= *task_ptr;
		int valveNumber = task.getValveNumber();
		if (valveNumber == -1) {
			println("No valve available");
			task.markAsCompleted();
			return false;
		}

		// println("Task available: ", task);

		// status.currentTaskName = task.name;
		// vm.setValveOperating(task.getValveNumber());

		handleTask(task);

		// while (true) {
		// 	KPStatus::current().preventShutdown = false;
		// 	int id								= app.availableTask();
		// 	if (id == -1) {
		// 		println("No task available");
		// 		power.disarmAlarms();
		// 		KPStatus::current().valveCurrent = -1;
		// 		return;
		// 	} else {
		// 		println("Task available:", id);
		// 		app.loadTaskFromStorage(id);
		// 		// Fix
		// 		// KPStatus::current().valveCurrent = app.currentTask.valves[app.currentTask.numberOfValvesAssigned - 1];
		// 		KPStatus::current().valveCurrent = app.currentTask.valves[0];
		// 	}

		// 	// If waking up before the task (ontrack),
		// 	// prevent shutdown if current time is within 10 secs of the task schedule else sleep until T-5
		// 	if (app.checkForMissedSchedule(app.currentTask)) {
		// 		if (app.taskWithin(app.currentTask, 10)) {
		// 			KPStatus::current().preventShutdown = true;
		// 			power.scheduleNextAlarm(app.currentTask.schedule);
		// 			println("Rescheduling with exact timing");
		// 		} else {
		// 			power.scheduleNextAlarm(app.currentTask.schedule - 5);
		// 			println("Rescheduling with 5secs before");
		// 		}
		// 		return;
		// 		// Missed the schedule but may not miss the deadline
		// 	} else if (app.beforeDeadline(app.currentTask, 5)) {
		// 		app.currentTask.schedule = now() + 5;
		// 		app.updateTaskFile(app.currentTask);
		// 		app.updateTaskrefsFile();
		// 		println("Before deadline: scheduling on the same valve...");
		// 	} else {
		// 		int lastValveIndex						   = app.currentTask.valves[app.currentTask.numberOfValvesAssigned - 1];
		// 		KPStatus::current().valves[lastValveIndex] = 1;
		// 		app.currentTask.next();
		// 		app.currentTask.schedule = now() + 5;
		// 		app.updateTaskFile(app.currentTask);
		// 		app.updateTaskrefsFile();

		// 		if (app.currentTask.numberOfValvesAssigned) {
		// 			println("Missed deadline: moving to next available valve...");
		// 		} else {
		// 			println("Missed deadline: searching for next available task...");
		// 		}
		// 	}
		// }
	}

	void update() override {
		KPController::update();

		if (status.isProgrammingMode()) {
			status.preventShutdown = true;
		}
	}
};