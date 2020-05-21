#pragma once

#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPSerialInput.hpp>
#include <KPServer.hpp>
#include <KPStateMachine.hpp>
#include <KPAction.hpp>

#include <Procedures/Main.hpp>

#include <Application/Config.hpp>
#include <Application/Constants.hpp>
#include <Application/Status.hpp>

#include <Components/Pump.hpp>
#include <Components/ShiftRegister.hpp>
#include <Components/Power.hpp>

#include <Valve/Valve.hpp>
#include <Valve/ValveManager.hpp>

#include <Task/Task.hpp>
#include <Task/TaskManager.hpp>

#include <Utilities/JsonEncodableDecodable.hpp>

#include <Application/Action.hpp>

#define AirValveBitIndex	 2
#define AlcoholValveBitIndex 3
#define FlushValveBitIndex	 5

extern void printDirectory(File dir, int numTabs);

inline bool checkForConnection(uint8_t addr) {
	Wire.begin();
	// Wire.beginTransmission(addr);
	// Wire.write(1);
	// Wire.endTransmission();
	Wire.requestFrom(addr, 1);
	return (Wire.read() != -1);
}

class Application : public KPController,
					public KPSerialInputListener,
					public TaskListener {
private:
	void setupServerRouting();
	void commandReceived(const String & line) override;

public:
	KPServer server{"web-server", "eDNA-test", "password"};
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

	Task * currentTask = nullptr;

private:
	void develop() {
		while (!Serial) {
			delay(100);
		};
	}

	void setup() override {
		KPController::setup();
		KPSerialInput::instance().addListener(this);

		Serial.begin(115200);
		develop();	// NOTE: Remove in production

		// Register state machine states
		addComponent(sm);
		sm.addListener(&status);
		sm.registerState(StateIdle(), StateName::IDLE);
		sm.registerState(StateStop(), StateName::STOP);
		sm.registerState(StateFlush(), StateName::FLUSH);
		sm.registerState(StateSample(), StateName::SAMPLE);
		sm.registerState(StateDry(), StateName::DRY);
		sm.registerState(StatePreserve(), StateName::PRESERVE);

		// Components; the order here doesn't matter
		addComponent(fileLoader);
		addComponent(shift);
		addComponent(power);
		addComponent(pump);
		addComponent(scheduler);
		addComponent(server);
		setupServerRouting();
		server.begin();

		// Load configuration from file and initialize status object
		config.load();
		status.init(config);

		// Config valve manager
		vm.init(config);
		vm.addListener(&status);
		vm.loadValvesFromDirectory(config.valveFolder);

		// Configure task manager
		tm.init(config);
		tm.loadTasksFromDirectory(config.taskFolder);

		// Schedule task if any then wait in IDLE
		scheduleNextActiveTask();
		sm.transitionTo(StateName::IDLE);

		// RTC Interrupt callback
		power.onInterrupt([this]() {
			scheduleNextActiveTask();
		});
	}

public:
	void transferTaskDataToStateParameters(Task & task) {
		auto & flush = *sm.getState<StateFlush>(StateName::FLUSH);
		flush.time	 = task.flushTime;

		auto & sample	= *sm.getState<StateSample>(StateName::SAMPLE);
		sample.time		= task.sampleTime;
		sample.pressure = task.samplePressure;

		auto & dry = *sm.getState<StateDry>(StateName::DRY);
		dry.time   = task.dryTime;

		auto & preserve = *sm.getState<StatePreserve>(StateName::PRESERVE);
		preserve.time	= task.preserveTime;
	}

	// Return true if task is successfully scheduled.
	// False if task is either missed or not available.
	bool scheduleNextActiveTask() {
		if (!(currentTask = tm.nearestActiveTask())) {
			power.disarmAlarms();
			status.currentValve = -1;
			println("No active task");
			return false;
		}

		// Retrieve task
		Task & task	   = *currentTask;
		time_t timenow = now();

		// Missed schedule. Clear remaining valves and mark Task as completed.
		if (timenow >= task.schedule) {
			clearRemainingValves(task);
			tm.markTaskAsCompleted(task);
			tm.writeTaskArrayToDirectory();
			currentTask = nullptr;
			println("Missed task schedule");
			return false;
		}

		// Waking up 10 secs before schedule time
		// Else if Waking up more than 10 secs before the schedule time,
		// reschedule 5 secs before actual time
		if (timenow >= task.schedule - 10) {
			auto delayTaskExecution = [this]() {
				sm.transitionTo(StateName::FLUSH);
			};

			run(secsToMillis(task.schedule - timenow), delayTaskExecution);
			transferTaskDataToStateParameters(task);
			println("Task executes in ", task.schedule - timenow, " secs");
		} else {
			power.scheduleNextAlarm(task.schedule - 5);
			println("Task reschduled to 5 secs before actual time");
		}

		// Set valve status to operating
		int valveIndex = task.getValveIndex();
		if (valveIndex != -1) {
			vm.setValveStatus(valveIndex, ValveStatus::operating);
		}

		return true;
	}

	//
	// ─── UPDATE ─────────────────────────────────────────────────────────────────────
	//
	// Runs after the setup and initialization of all components
	// ────────────────────────────────────────────────────────────────────────────────
	void update() override {
		KPController::update();

		// Shutdown if we are not in progrmaming mode and preventShutdown=false
		if (!status.isProgrammingMode() && !status.preventShutdown) {
			shutdown();
		}
	}

	void shutdown() {
		pump.off();				// Turn off motor
		shift.writeZeros();		// Turn off all TPIC devices
		shift.writeLatchOut();	// Reverse latch valve direction
		power.shutdown();		// Turn off power
	}

	void clearRemainingValves(Task & task) {
		for (int i = task.currentValveIndex; i < task.valveCount; i++) {
			vm.freeIfNotYetSampled(task.valves[i]);
		}
	}

	void taskChanged(Task & task, TaskManager & tm) override {
	}
};