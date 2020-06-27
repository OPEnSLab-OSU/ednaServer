#pragma once
#define ARDUINOJSON_USE_LONG_LONG 1
#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPSerialInput.hpp>
#include <KPServer.hpp>
#include <KPStateMachine.hpp>
#include <Action.hpp>

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

#define PRINT_MODE(mode, x)                        \
	PrintConfig::setPrintVerbose(Verbosity::mode); \
	x;                                             \
	PrintConfig::setDefaultVerbose()

#define PRINT_DEBUG	  PrintConfig::setPrintVerbose(Verbosity::debug);
#define PRINT_DEFAULT PrintConfig::setDefaultVerbose();

extern void printDirectory(File dir, int numTabs);
inline bool checkForConnection(uint8_t addr) {
	Wire.begin();
	// Wire.beginTransmission(addr);
	// Wire.write(1);
	// Wire.endTransmission();
	Wire.requestFrom(addr, 1);
	return (Wire.read() != -1);
}

struct ValveBlock {
	const int regIndex;
	const int pinIndex;
};

class Application : public KPController,
					public KPSerialInputListener,
					public TaskObserver {
private:
	void setupServerRouting();
	void commandReceived(const String & line) override;

public:
	KPServer server{"web-server",
		"eDNA-test",
		"password"};

	Pump pump{"pump",
		HardwarePins::MOTOR_FORWARD,
		HardwarePins::MOTOR_REVERSE};

	ShiftRegister shift{"shift-register",
		32,
		HardwarePins::SHFT_REG_DATA,
		HardwarePins::SHFT_REG_CLOCK,
		HardwarePins::SHFT_REG_LATCH};

	KPFileLoader fileLoader{"file-loader",
		HardwarePins::SD_CARD};

	KPStateMachine sm{"state-machine"};
	Power power{"power"};

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

#ifdef DEBUG
		PRINT_MODE(debug, println("DEBUG MODE"));
		println("Default print verbosity is ", static_cast<int>(PrintConfig::defaultPrintVerbose));
#endif

		// Registering components. The order here should not matter
		addComponent(sm);
		addComponent(fileLoader);
		addComponent(shift);
		addComponent(power);
		addComponent(pump);
		addComponent(server);
		server.begin();

		// Register states
		sm.addListener(&status);
		sm.registerState(StateIdle(), StateName::IDLE);
		sm.registerState(StateStop(), StateName::STOP);
		sm.registerState(StateFlush(), StateName::FLUSH);
		sm.registerState(StateSample(), StateName::SAMPLE);
		sm.registerState(StateDry(), StateName::DRY);
		sm.registerState(StatePreserve(), StateName::PRESERVE);

		// Delay the server setup to reduce startup time
		run(0, [this]() {
			setupServerRouting();
		});

		// Load configuration from file and initialize status object
		config.load();
		status.init(config);

		// Configure valve manager
		vm.init(config);
		vm.addObserver(status);
		vm.loadValvesFromDirectory(config.valveFolder);

		// Configure task manager
		tm.init(config);
		tm.loadTasksFromDirectory(config.taskFolder);

		// Schedule task if any then wait in IDLE
		scheduleNextActiveTask();
		sm.transitionTo(StateName::IDLE);

		// RTC Interrupt callback
		power.onInterrupt([this]() {
			println("\033[1;32mRTC Interrupted!\033[0m");
			power.disarmAlarms();
			scheduleNextActiveTask();
		});
	}

public:
	ValveBlock currentValveNumberToBlock() {
		return {
			shift.toRegisterIndex(status.currentValve) + 1,
			shift.toPinIndex(status.currentValve)};
	}
	// ────────────────────────────────────────────────────────────────────────────────
	// Decouple state machine from task object. This is where task data gets transfered
	// to states' parameters
	// ────────────────────────────────────────────────────────────────────────────────
	void transferTaskDataToStateParameters(Task & task) {
		auto & flush = *sm.getState<StateFlush>(StateName::FLUSH);
		flush.time	 = task.flushTime;
		flush.volume = task.flushVolume;

		auto & sample	= *sm.getState<StateSample>(StateName::SAMPLE);
		sample.time		= task.sampleTime;
		sample.pressure = task.samplePressure;
		sample.volume	= task.flushVolume;

		auto & dry = *sm.getState<StateDry>(StateName::DRY);
		dry.time   = task.dryTime;

		auto & preserve = *sm.getState<StatePreserve>(StateName::PRESERVE);
		preserve.time	= task.preserveTime;
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Return:
	//     True if task is successfully scheduled.
	//     False if task is either missed schedule or no active task available.
	// ────────────────────────────────────────────────────────────────────────────────
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
			PRINT_MODE(info, println("Missed schedule"));
			return false;
		}

		// If this method is called between 10 secs before the actual schedule,
		// then stays awake until the task execution.
		// Else, reschedule RTC interrupt to 5 secs before the actual schedule.
		if (timenow >= task.schedule - 10) {
			TimedAction delayTaskExecution;
			delayTaskExecution.name		= "delayTaskExecution";
			delayTaskExecution.interval = secsToMillis(task.schedule - timenow);
			delayTaskExecution.callback = [this]() {
				sm.transitionTo(StateName::FLUSH);
			};

			// ASYNC: Notice how we are delaying this function call
			run(delayTaskExecution);
			transferTaskDataToStateParameters(task);

			PRINT_MODE(debug, println("Task executes in ", task.schedule - timenow, " secs"));
		} else {
			power.scheduleNextAlarm(task.schedule - 5);
			PRINT_MODE(debug, println("Task reschduled to 5 secs before actual time"));
		}

		// Set valve status to operating
		int valveIndex = task.getCurrentValveIndex();
		if (valveIndex != -1) {
			vm.setValveStatus(valveIndex, ValveStatus::operating);
		}

		return true;
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 ** @brief Runs after the setup and initialization of all components
	 ** 
	───────────────────────────────────────────────────────────────────────────── */
	void update() override {
		KPController::update();

		// Shutdown if we are not in progrmaming mode and preventShutdown=false
		if (!status.isProgrammingMode() && !status.preventShutdown) {
			shutdown();
		}
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 ** @brief Turn off the motor, shut off the pins and power off the system
	 **
	───────────────────────────────────────────────────────────────────────────── */
	void shutdown() {
		pump.off();					   // Turn off motor
		shift.writeAllRegistersLow();  // Turn off all TPIC devices
		shift.writeLatchOut();		   // Reverse latch valve direction
		power.shutdown();			   // Turn off power
	}

	void clearRemainingValves(Task & task) {
		for (int i = task.currentValvePosition(); i < task.size(); i++) {
			vm.setValveFreeIfNotYetSampled(task.valves[i]);
		}
	}

	void taskDidUpdate(const Task & task) override {
	}

	void taskCollectionDidUpdate(const std::vector<Task> & tasks) override {
	}
};