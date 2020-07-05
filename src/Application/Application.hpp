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

#define PRINT_REGION_DEBUG PrintConfig::setPrintVerbose(Verbosity::debug);
#define PRINT_DEFAULT	   PrintConfig::setDefaultVerbose();

extern void printDirectory(File dir, int numTabs);
inline bool checkForConnection(uint8_t addr) {
	Wire.begin();
	Wire.requestFrom(addr, 1);
	return (Wire.read() != -1);
}

struct ValveBlock {
	const int regIndex;
	const int pinIndex;
};

enum class ScheduleStatus {
	noActiveTask,
	inOperation,
	onSchedule,
};

class Application : public KPController, public KPSerialInputObserver, public TaskObserver {
private:
	void setupServerRouting();
	void commandReceived(const String & line) override;

public:
	KPServer server{"web-server", "eDNA-test", "password"};

	Pump pump{"pump", HardwarePins::MOTOR_FORWARD, HardwarePins::MOTOR_REVERSE};

	ShiftRegister shift{"shift-register",
		32,
		HardwarePins::SHFT_REG_DATA,
		HardwarePins::SHFT_REG_CLOCK,
		HardwarePins::SHFT_REG_LATCH};

	KPFileLoader fileLoader{"file-loader", HardwarePins::SD_CARD};

	KPStateMachine sm{"state-machine"};
	Power power{"power"};

	Config config{ProgramSettings::CONFIG_FILE_PATH};
	Status status;

	ValveManager vm;
	TaskManager tm;

	int currentTaskId = 0;

private:
	const char * KPSerialInputObserverName() const override {
		return "Application-KPSerialInput Observer";
	}

	const char * TaskObserverName() const override {
		return "Application-Task Observer";
	}

	void develop() {
		while (!Serial) {
			delay(100);
		};
	}

	void setup() override {
		KPSerialInput::sharedInstance().addObserver(this);

		Serial.begin(115200);
		develop();	// NOTE: Remove in production

		// Here we add and initialize the power module first. This allows us to get the
		// actual time from the RTC for random task id generation
		addComponent(power);
		randomSeed(now());

		PRINT_REGION_DEBUG
		println("DEBUG MODE");
		println("Default print verbosity is ", static_cast<int>(PrintConfig::defaultPrintVerbose));
		PRINT_DEFAULT

		// Register components
		// Broadcast the WIFI signal as soon as possible
		addComponent(server);
		server.begin();
		run(0, [this]() { setupServerRouting(); });

		addComponent(sm);
		addComponent(fileLoader);
		addComponent(shift);
		addComponent(pump);

		// Register states
		sm.addObserver(status);
		sm.registerState(StateIdle(), StateName::IDLE);
		sm.registerState(StateStop(), StateName::STOP);
		sm.registerState(StateFlush(), StateName::FLUSH);
		sm.registerState(StateSample(), StateName::SAMPLE);
		sm.registerState(StateDry(), StateName::DRY);
		sm.registerState(StatePreserve(), StateName::PRESERVE);

		// Load configuration from file and initialize config then status object
		JsonFileLoader loader;
		loader.load(config.configFilepath, config);
		status.init(config);

		// Configure valve manager
		vm.init(config);
		vm.addObserver(status);
		vm.loadValvesFromDirectory(config.valveFolder);

		// Configure task manager
		tm.init(config);
		tm.addObserver(this);
		tm.loadTasksFromDirectory(config.taskFolder);

		// Schedule task if any then wait in IDLE
		ScheduleStatus returnedCode = scheduleNextActiveTask();
		println("Scheduling returned code: ", static_cast<int>(returnedCode));
		sm.transitionTo(StateName::IDLE);

		// RTC Interrupt callback
		power.onInterrupt([this]() {
			println("\033[1;32mRTC Interrupted!\033[0m");
			power.disarmAlarms();
			ScheduleStatus returnedCode = scheduleNextActiveTask();
			println("Scheduling returned code: ", static_cast<int>(returnedCode));
		});

		runForever(2000, "mem", []() { printFreeRam(); });
	}

public:
	ValveBlock currentValveNumberToBlock() {
		return {shift.toRegisterIndex(status.currentValve) + 1,
			shift.toPinIndex(status.currentValve)};
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Decouple state machine from task object. This is where task data gets
	 *  transfered to states' parameters
	 *
	 *  @param task Task object containing states' parameters
	 *  ──────────────────────────────────────────────────────────────────────────── */
	void transferTaskDataToStateParameters(const Task & task) {
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

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Get the earliest upcoming task and schedule it
	 *
	 *  @return true if task is successfully scheduled.
	 *  @return false if task is either missed schedule or no active task available.
	 *  ──────────────────────────────────────────────────────────────────────────── */
	ScheduleStatus scheduleNextActiveTask(bool shouldInvalidateCurrentTask = false) {
		std::vector<int> taskIds = tm.getActiveSortedTaskIds();
		if (taskIds.empty()) {
			return ScheduleStatus::noActiveTask;
		}

		for (auto id : taskIds) {
			auto & task		= tm.tasks[id];
			time_t time_now = now();

			if (id == currentTaskId) {
				if (time_now >= task.schedule && shouldInvalidateCurrentTask) {
					cancel("delayTaskExecution");
					invalidateTask(task);
					if (status.currentStateName != StateName::STOP) {
						sm.transitionTo(StateName::STOP);
					}

					continue;
				}

				return ScheduleStatus::inOperation;
			}

			// Missed schedule
			if (time_now >= task.schedule) {
				println(RED("Missed schedule"));
				invalidateTask(task);
				tm.writeToDirectory();
				continue;
			}

			// Wake up between 10 secs of the actual schedule time
			// Prepare an action and return status=operating
			if (time_now >= task.schedule - 10) {
				TimedAction delayTaskExecution;
				delayTaskExecution.name		= "delayTaskExecution";
				delayTaskExecution.interval = secsToMillis(task.schedule - time_now);
				delayTaskExecution.callback = [this]() { sm.transitionTo(StateName::FLUSH); };

				run(delayTaskExecution);
				transferTaskDataToStateParameters(task);
				status.preventShutdown = true;
				currentTaskId		   = id;
				vm.setValveStatus(task.valves[task.valveOffsetStart], ValveStatus::operating);
				println("Executing task in", task.schedule - time_now, "seconds");
				return ScheduleStatus::inOperation;
			}

			// Wake up before any task
			status.preventShutdown = false;
			power.scheduleNextAlarm(task.schedule - 5);
			return ScheduleStatus::onSchedule;
		}

		currentTaskId = 0;
		return ScheduleStatus::noActiveTask;
	}

	void validateTaskForSaving(const Task & task, JsonDocument & response) {
		if (task.status == 1) {
			response["error"] = "Task is current active";
			return;
		}

		if (!tm.findTask(task.id)) {
			response["error"] = "Task not found: invalid task id";
			return;
		}
	}

	void validateTaskForScheduling(const Task & task) {}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Runs after the setup and initialization of all components
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	void update() override {
		KPController::update();

		if (!status.isProgrammingMode() && !status.preventShutdown) {
			shutdown();
		}
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Turn off the motor, shut off the pins and power off the system
	 *
	 *  ──────────────────────────────────────────────────────────────────────────── */
	void shutdown() {
		pump.off();					   // Turn off motor
		shift.writeAllRegistersLow();  // Turn off all TPIC devices
		shift.writeLatchOut();		   // Reverse latch valve direction
		tm.writeToDirectory();
		vm.writeToDirectory();
		power.shutdown();
		raise("SHUTDOWN");
	}

	void invalidateTask(Task & task) {
		for (int i = task.getValveOffsetStart(); i < task.getNumberOfValves(); i++) {
			vm.setValveFreeIfNotYetSampled(task.valves[i]);
		}

		task.valves.clear();
		tm.markTaskAsCompleted(task.id);
	}

	void taskDidUpdate(const Task & task) override {}

	void taskDidDelete(int id) override {
		if (currentTaskId == id) {
			currentTaskId = 0;
		}
	}

	void taskCollectionDidUpdate(const std::vector<Task> & tasks) override {}
};