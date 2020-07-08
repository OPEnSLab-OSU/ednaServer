#pragma once
#define ARDUINOJSON_USE_LONG_LONG 1
#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPSerialInput.hpp>
#include <KPServer.hpp>
#include <Action.hpp>

#include <Application/Config.hpp>
#include <Application/Constants.hpp>
#include <Application/Status.hpp>
#include <Application/ScheduleReturnCode.hpp>

#include <Components/Pump.hpp>
#include <Components/ShiftRegister.hpp>
#include <Components/Power.hpp>
#include <Components/SensorArray.hpp>

#include <Procedures/MainStateMachine.hpp>
#include <Procedures/BallStateMachine.hpp>
#include <Procedures/NewStateMachine.hpp>

#include <Valve/Valve.hpp>
#include <Valve/ValveManager.hpp>

#include <Task/Task.hpp>
#include <Task/TaskManager.hpp>

#include <Utilities/JsonEncodableDecodable.hpp>

extern void printDirectory(File dir, int numTabs);

class Application : public KPController, public KPSerialInputObserver, public TaskObserver {
private:
	void setupServerRouting();
	void commandReceived(const String & line) override;

public:
	KPFileLoader fileLoader{"file-loader", HardwarePins::SD_CARD};
	KPServer server{"web-server", "eDNA-test", "password"};

	Pump pump{
		"pump",
		HardwarePins::MOTOR_FORWARD,
		HardwarePins::MOTOR_REVERSE,
	};

	const int numberOfShiftRegisters = 4;
	ShiftRegister shift{"shift-register",
						numberOfShiftRegisters,
						HardwarePins::SHFT_REG_DATA,
						HardwarePins::SHFT_REG_CLOCK,
						HardwarePins::SHFT_REG_LATCH};

	Power power{"power"};

	Config config{ProgramSettings::CONFIG_FILE_PATH};
	Status status;

	// MainStateMachine sm;
	BallStateMachine sm;

	ValveManager vm;
	TaskManager tm;

	SensorArray sensors{"sensor-array"};

	int currentTaskId = 0;

private:
	const char * KPSerialInputObserverName() const override {
		return "Application-KPSerialInput Observer";
	}

	const char * TaskObserverName() const override {
		return "Application-Task Observer";
	}

	void setup() override {
		KPSerialInput::sharedInstance().addObserver(this);
		Serial.begin(115200);

#ifndef RELEASE
		while (!Serial) {};
#endif
		// Here we add and initialize the power module first. This allows us to get the
		// actual time from the RTC for random task id generation
		addComponent(power);
		randomSeed(now());

		PRINT_REGION_DEBUG
		println();
		println("==================================================");
		println("DEBUG MODE");
		println("Default print verbosity is ", static_cast<int>(PrintConfig::defaultPrintVerbose));
		println("==================================================");
		PRINT_DEFAULT

		// Register server and broadcast the WIFI signal as soon as possible
		addComponent(server);
		server.begin();
		setupServerRouting();

		// Register the rest of the components
		addComponent(KPSerialInput::sharedInstance());
		addComponent(ActionScheduler::sharedInstance());
		addComponent(sm);
		addComponent(fileLoader);
		addComponent(shift);
		addComponent(pump);
		addComponent(sensors);
		sensors.addObserver(status);
		sm.addObserver(status);

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

		// Wait in IDLE
		sm.idle();

		// RTC Interrupt callback
		power.onInterrupt([this]() {
			println(GREEN("RTC Interrupted!"));
			println(scheduleNextActiveTask().description());
		});

#ifdef DEBUG
		runForever(2000, "mem", []() { printFreeRam(); });
#endif
	}

public:
	/** ────────────────────────────────────────────────────────────────────────────
	 *  Convenient method for working with latch valve. Leaving the hardware
	 *  implementer to decide what is "normal" direction.
	 *
	 *  @param controlPin Control pin
	 *  ──────────────────────────────────────────────────────────────────────────── */
	void writeIntakeValve(ValveDirection direction = ValveDirection::normal) {
		int controlPin = 0, reversePin = 1;
		if (direction == ValveDirection::normal) {
			std::swap(controlPin, reversePin);
		}

		shift.setPin(controlPin, HIGH);
		shift.setPin(reversePin, LOW);
		shift.write();
	}

	ValveBlock currentValveNumberToBlock() {
		return {shift.toRegisterIndex(status.currentValve) + 1,
				shift.toPinIndex(status.currentValve)};
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Get the earliest upcoming task and schedule it
	 *
	 *  @return true if task is successfully scheduled.
	 *  @return false if task is either missed schedule or no active task available.
	 *  ──────────────────────────────────────────────────────────────────────────── */
	ScheduleReturnCode scheduleNextActiveTask(bool shouldStopCurrentTask = false) {
		for (auto id : tm.getActiveSortedTaskIds()) {
			Task & task		= tm.tasks[id];
			time_t time_now = now();

			if (currentTaskId == id) {
				// NOTE: Check logic here. Maybe not be correct yet
				if (shouldStopCurrentTask) {
					cancel("delayTaskExecution");
					if (status.currentStateName != sm.stopStateName) {
						sm.stop();
					}

					continue;
				} else {
					return ScheduleReturnCode::operating;
				}
			}

			if (time_now >= task.schedule) {
				// Missed schedule
				println(RED("Missed schedule"));
				invalidateTaskAndFreeUpValves(task);
				continue;
			}

			if (time_now >= task.schedule - 10) {
				// Wake up between 10 secs of the actual schedule time
				// Prepare an action to execute at exact time
				const auto timeUntil = task.schedule - time_now;

				TimedAction delayTaskExecution;
				delayTaskExecution.name		= "delayTaskExecution";
				delayTaskExecution.interval = secsToMillis(timeUntil);
				delayTaskExecution.callback = [this]() { sm.begin(); };
				run(delayTaskExecution);  // async, will be execute later

				sm.transferTaskDataToStateParameters(task);

				currentTaskId		   = id;
				status.preventShutdown = true;
				vm.setValveStatus(task.valves[task.valveOffsetStart], ValveStatus::operating);

				println("\033[32;1mExecuting task in ", timeUntil, " seconds\033[0m");
				return ScheduleReturnCode::operating;
			} else {
				// Wake up before not due to alarm, reschedule anyway
				status.preventShutdown = false;
				power.scheduleNextAlarm(task.schedule - 8);	 // 3 < x < 10
				return ScheduleReturnCode::scheduled;
			}
		}

		currentTaskId = 0;
		return ScheduleReturnCode::unavailable;
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

	void validateTaskForScheduling(int id, JsonDocument & response) {
		if (!tm.findTask(id)) {
			response["error"] = "Task not found";
			return;
		}

		Task & task = tm.tasks[id];
		if (task.getNumberOfValves() == 0) {
			response["error"] = "Cannot schedule a task without an assigned valve";
			return;
		}

		if (task.schedule <= now() + 3) {
			response["error"] = "Must be in the future";
			return;
		}

		for (int v : task.valves) {
			if (vm.valves[v].status == ValveStatus::sampled) {
				KPStringBuilder<100> error("Valve ", v, " has already been sampled");
				response["error"] = error;
				return;
			}
		}
	}

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
		pump.off();									// Turn off motor
		shift.writeAllRegistersLow();				// Turn off all TPIC devices
		writeIntakeValve(ValveDirection::reverse);	// Reverse intake valve

		tm.writeToDirectory();
		vm.writeToDirectory();
		power.shutdown();
		raise("SHUTDOWN");
	}

	void invalidateTaskAndFreeUpValves(Task & task) {
		for (int i = task.getValveOffsetStart(); i < task.getNumberOfValves(); i++) {
			vm.setValveFreeIfNotYetSampled(task.valves[i]);
		}

		task.valves.clear();
		tm.markTaskAsCompleted(task.id);
	}

private:
	void taskDidUpdate(const Task & task) override {}

	void taskDidDelete(int id) override {
		if (currentTaskId == id) {
			currentTaskId = 0;
		}
	}
};