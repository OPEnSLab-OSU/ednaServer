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

private:
	void develop() {
		while (!Serial) {
			delay(100);
		};
		// delay(5000);
	}

	void setup() override {
		KPController::setup();
		KPSerialInput::instance().addListener(this);

		Serial.begin(115200);
		develop();	// NOTE: Remove on production

		// Register state machine states
		addComponent(sm);
		sm.addListener(&status);
		sm.registerState(StateIdle(), StateName::IDLE);
		sm.registerState(StateStop(), StateName::STOP);
		sm.registerState(StateFlush(), StateName::FLUSH);
		sm.registerState(StateSample(), StateName::SAMPLE);
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
		scheduleNextAvailableTask();
		sm.transitionTo(StateName::IDLE);

		// RTC Interrupt callback
		power.onInterrupt([]() {
			println("Alarm Triggered");
		});
	}

	void transferTaskDataToStateParameters(Task & task) {
		auto & stateFlush = *sm.getState<StateFlush>(StateName::FLUSH);
		stateFlush.time	  = task.flushTime;
		stateFlush.volume = task.flushVolume;

		// blah blah blah
	}

	bool handleTaskScheduling(Task & task) {
		time_t timenow = now();

		// NOTE: Missed schedule. Invalidate Task.
		if (timenow >= task.schedule) {
			for (int valve_idx : task.valves) {
				vm.freeIfNotYetSampled(valve_idx);
				task.markAsCompleted();
			}

			return false;
		}

		// NOTE: Waking up 10 secs before schedule time
		if (timenow >= task.schedule - 10) {
			transferTaskDataToStateParameters(task);
			auto wait = [this]() { sm.transitionTo(StateName::FLUSH); };
			run(secsToMillis(task.schedule - timenow), wait, scheduler);
			config.shutdownOverride = true;
		}

		// NOTE: Waking up more than 10 secs before the schedule time
		// Reschedule 5 secs before due
		else {
			power.scheduleNextAlarm(task.schedule - 5);
		}

		return true;
	}

	bool scheduleNextAvailableTask() {
		Task * task_ptr = tm.next();
		if (!task_ptr) {
			println("No task available");
			power.disarmAlarms();
			status.valveCurrent = -1;
			return false;
		}

		return handleTaskScheduling(*task_ptr);
	}

	void shutdown() {
		pump.off();				// Turn off motor
		shift.writeZeros();		// Turn off all TPIC devices
		shift.writeLatchOut();	// Reverse latch valve direction
		power.shutdown();		// Turn off power
	}

	//
	// ─── UPDATE ─────────────────────────────────────────────────────────────────────
	//
	// Runs after the setup and initialization of all components
	// ────────────────────────────────────────────────────────────────────────────────
	void update() override {
		KPController::update();

		// Prevent shutdown if we are in programming momde and config has override
		if (status.isProgrammingMode() || config.shutdownOverride) {
			status.preventShutdown = true;
		}

		// Shutdown
		if (!status.preventShutdown) {
			shutdown();
		}
	}
};