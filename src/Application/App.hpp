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
#include <Components/Intake.hpp>

#include <StateControllers/NewStateController.hpp>
#include <StateControllers/HyperFlushStateController.hpp>
#include <StateControllers/DebubbleStateController.hpp>

#include <Valve/Valve.hpp>
#include <Valve/ValveManager.hpp>

#include <Task/Task.hpp>
#include <Task/TaskManager.hpp>

#include <Utilities/JsonEncodableDecodable.hpp>

#include <API/API.hpp>

#include <configuration.hpp>

class App : public KPController, public KPSerialInputObserver, public TaskObserver {
private:
    void setupAPI();
    void setupSerialRouting();
    void setupServerRouting();
//    void testValve(int v);
    void commandReceived(const char * line, size_t size) override;

public:
    KPFileLoader fileLoader{"file-loader", HardwarePins::SD_CARD};
    KPServer server{"web-server", SERVER_NAME, SERVER_PASSWORD};

    Pump pump{
        "pump",
        HardwarePins::MOTOR_FORWARD,
        HardwarePins::MOTOR_REVERSE,
    };

    const int numberOfRegisters = 4;
    ShiftRegister shift{
        "shift-register",
        numberOfRegisters,
        HardwarePins::SHFT_REG_DATA,
        HardwarePins::SHFT_REG_CLOCK,
        HardwarePins::SHFT_REG_LATCH,
    };

    Power power{"power"};
    BallIntake intake{shift};
    Config config{ProgramSettings::CONFIG_FILE_PATH};
    Status status;

    // MainStateController sm;
    NewStateController newStateController;
    HyperFlushStateController hyperFlushStateController;
    DebubbleStateController debubbleStateController;

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


    void testValve(int v, const char * name) {
            println("=========");
            print("Testing valve: ");
            print(name);
            println();
            shift.setAllRegistersLow();
            shift.writePin(v + shift.capacityPerRegister, HIGH);
            shift.write();
            println("press any key to continue: ");
              while (!Serial.available()) {
                yield();
            }
            while(Serial.available() > 0){
                Serial.read();
            }
            delay(20);
    }

public:
    void setup() override {
        KPSerialInput::sharedInstance().addObserver(this);
        Serial.begin(115200);

#if defined(DEBUG) || defined(COMPONENT_TEST)
        while (!Serial) {};
#endif
#ifdef DEBUG
        println();
        println(BLUE("=================================================="));
        println(BLUE("                   DEBUG MODE"));
        println(BLUE("=================================================="));
#endif

        //
        // ─── POWER MODULE ────────────────────────────────────────────────
        //
        // Here we add and initialize the power module first.
        // So we can seed the random number generator with actual time from RTC.
        addComponent(power);
        randomSeed(now());

        //
        // ─── ADD WIFI SERVER ─────────────────────────────────────────────
        //

        addComponent(server);
        server.begin();
        setupServerRouting();

        //
        // ─── ADDING COMPONENTS ───────────────────────────────────────────
        //

        addComponent(KPSerialInput::sharedInstance());
        setupSerialRouting();

        addComponent(ActionScheduler::sharedInstance());
        addComponent(fileLoader);
        addComponent(shift);
        addComponent(pump);
        addComponent(sensors);
        sensors.addObserver(status);

        //
        // ─── LOADING CONFIG FILE ─────────────────────────────────────────
        //
        // Load configuration from file to initialize config and status objects
        JsonFileLoader loader;
        loader.load(config.configFilepath, config);
        status.init(config);

        //
        // ─── ADDING VALVE MANAGER ────────────────────────────────────────
        //

        vm.init(config);
        vm.addObserver(status);
        vm.loadValvesFromDirectory(config.valveFolder);

        //
        // ─── ADDING TASK MANAGER ─────────────────────────────────────────
        //

        tm.init(config);
        tm.addObserver(this);
        tm.loadTasksFromDirectory(config.taskFolder);

        //
        // ─── HYPER FLUSH CONTROLLER ──────────────────────────────────────
        //

        hyperFlushStateController.configure([](HyperFlush::Config & config) {
            config.flushTime   = 5;
            config.preloadTime = 5;
        });

        addComponent(hyperFlushStateController);
        hyperFlushStateController.idle();  // Wait in IDLE

        //
        // ─── Debubbler CONTROLLER ──────────────────────────────────────
        //

        debubbleStateController.configure([](Debubble::Config & config) {
            config.time   = 10;
        });

        addComponent(debubbleStateController);
        debubbleStateController.idle();  // Wait in IDLE

        //
        // ─── NEW STATE CONTROLLER ────────────────────────────────────────
        //

        addComponent(newStateController);
        newStateController.addObserver(status);
        newStateController.idle();  // Wait in IDLE

        // Print WiFi status
        if (server.enabled()) {
            println();
            println(BLUE("====================== WIFI ======================"));
            server.printWiFiStatus();
            println(BLUE("=================================================="));
        }

        // Regular log header
        if (!SD.exists(config.logFile)) {
            File file = SD.open(config.logFile, FILE_WRITE);
            KPStringBuilder<404> header{"UTC, Formatted Time, Task Name, Valve Number, Current "
                                        "State, Config Sample Time, Config Sample "
                                        "Pressure, Config Sample Volume, Temperature Recorded,"
                                        "Max Pressure Recorded, Volume Recorded, Flow Rate\n"};
            file.println(header);
            file.close();
        }

        // Detail log header
        if (!SD.exists("detail.csv")) {
            File file = SD.open("detail.csv", FILE_WRITE);
            KPStringBuilder<404> header{"UTC, Formatted Time, Task Name, Valve Number, Current "
                                        "State, Config Sample Time, Config Sample "
                                        "Pressure, Config Sample Volume, Temperature Recorded,"
                                        "Pressure Recorded, Volume Recorded, Flow Rate\n"};
            file.println(header);
            file.close();
        }

        // RTC Interrupt callback
        power.onInterrupt([this]() {
            println(GREEN("RTC Interrupted!"));
            println(scheduleNextActiveTask().description());
        });

        runForever(1000, "detailLog", [&]() { logDetail("detail.csv"); });
#if defined(DEBUG) || defined(COMPONENT_TEST)
        runForever(2000, "memLog", [&]() { printFreeRam(); });
#endif

#ifdef COMPONENT_TEST
        println();
        println(BLUE("=================== RUNNING COMPONENT TEST =================="));

        println("Press any key to start: ");
        while (!Serial.available()) {
            yield();
        }
        while(Serial.available() > 0){
            Serial.read();
        }
        char buffer [50];
        for(int i = 0; i < 24; i++){
            std::sprintf(buffer, "%d", i);
            testValve(i, buffer);
        }

        testValve(TPICDevices::AIR_VALVE, "Air valve");
        testValve(TPICDevices::FLUSH_VALVE, "Flush valve");
        testValve(TPICDevices::ALCHOHOL_VALVE, "Alcohol valve");

        println("Testing ball valve");

        shift.writeAllRegistersLow();
        intake.on();
        println("press any key to continue: ");
        while (!Serial.available()) {
            yield();
        }
        while(Serial.available() > 0){
            Serial.read();
        }
        intake.off();

        println();
        println("Testing pump...");
        shift.writeAllRegistersLow();
        shift.setPin(TPICDevices::AIR_VALVE, HIGH);
        shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        shift.write();
        pump.on();
        delay(3000);
        pump.off();
        delay(3000);
        pump.on(Direction::reverse);
        delay(3000);
        pump.off();
        println("press any key to continue: ");
        while (!Serial.available()) {
            yield();
        }
        while(Serial.available() > 0){
            Serial.read();
        }

        if(sensors.pressure.enabled){
            println("Pressure sensor detected");
        } else {
            println(RED("Pressure sensor not detected"));
        }
        if(sensors.baro1.enabled){
            println("Baro1 sensor detected");
        }else{
            println(RED("Baro1 sensor not detected"));
        }
        if(sensors.baro2.enabled){
            println("Baro2 sensor detected");
        }else{
            println(RED("Baro2 sensor not detected"));
        }

        println(BLUE("=================== COMPONENT TEST COMPLETE =================="));
#endif
    }

    void logDetail(const char * filename) {
        if (currentTaskId) {
            SD.begin(HardwarePins::SD_CARD);
            File log    = SD.open(filename, FILE_WRITE);
            Task & task = tm.tasks.at(currentTaskId);

            char formattedTime[64];
            auto utc = now();
            sprintf(
                formattedTime, "%u/%u/%u %02u:%02u:%02u GMT+0", year(utc), month(utc), day(utc),
                hour(utc), minute(utc), second(utc));

            KPStringBuilder<544> data{
                utc,
                ",",
                formattedTime,
                ",",
                task.name,
                ",",
                status.currentValve,
                ",",
                status.currentStateName,
                ",",
                task.sampleTime,
                ",",
                task.samplePressure,
                ",",
                task.sampleVolume,
                ",",
                status.temperature,
                ",",
                status.pressure,
                ",",
                status.waterVolume,
                ",",
                status.waterFlow};
            log.println(data);
            log.flush();
            log.close();
        }
    }

    void logAfterSample() {
        SD.begin(HardwarePins::SD_CARD);
        File log    = SD.open(config.logFile, FILE_WRITE);
        Task & task = tm.tasks.at(currentTaskId);

        char formattedTime[64];
        auto utc = now();
        sprintf(
            formattedTime, "%u/%u/%u %02u:%02u:%02u GMT+0", year(utc), month(utc), day(utc),
            hour(utc), minute(utc), second(utc));

        KPStringBuilder<544> data{
            utc,
            ",",
            formattedTime,
            ",",
            task.name,
            ",",
            status.currentValve,
            ",",
            status.currentStateName,
            ",",
            task.sampleTime,
            ",",
            task.samplePressure,
            ",",
            task.sampleVolume,
            ",",
            status.temperature,
            ",",
            status.maxPressure,
            ",",
            status.waterVolume,
            ",",
            status.waterFlow};
        log.println(data);
        log.flush();
        log.close();
    }

    template <typename T, typename... Args>
    auto dispatchAPI(Args &&... args) {
        return T{}(*this, std::forward<Args>(args)...);
    }

    bool compare(const char * lhs, const char * rhs) {
        return strcmp(lhs, rhs) == 0;
    }

public:
    void beginHyperFlush() {
        hyperFlushStateController.begin();
    }

    void beginDebubble() {
        debubbleStateController.begin();
    }

    ValveBlock currentValveNumberToBlock() {
        return {
            shift.toRegisterIndex(status.currentValve) + 1, shift.toPinIndex(status.currentValve)};
    }

    int currentValveIdToPin() {
        return status.currentValve + shift.capacityPerRegister;  // <-- Skip the first register
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief Get the earliest upcoming task and schedule it
     *
     *  @return true if task is successfully scheduled.
     *  @return false if task is either missed schedule or no active task available.
     *  ──────────────────────────────────────────────────────────────────────────── */
    ScheduleReturnCode scheduleNextActiveTask(bool shouldStopCurrentTask = false) {
        status.preventShutdown = false;
        for (auto id : tm.getActiveSortedTaskIds()) {
            Task & task     = tm.tasks[id];
            time_t time_now = now();

            if (currentTaskId == id) {
                // NOTE: Check logic here. Maybe not be correct yet
                if (shouldStopCurrentTask) {
                    cancel("delayTaskExecution");
                    // if (status.currentStateName != HyperFlush::STOP) {
                    // 	newStateController.stop();
                    // }

                    continue;
                } else {
                    status.preventShutdown = true;
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
                delayTaskExecution.name     = "delayTaskExecution";
                delayTaskExecution.interval = secsToMillis(timeUntil);
                delayTaskExecution.callback = [this]() { newStateController.begin(); };
                run(delayTaskExecution);  // async, will be execute later

                newStateController.configure(task);

                currentTaskId          = id;
                status.preventShutdown = true;
                vm.setValveStatus(task.valves[task.valveOffsetStart], ValveStatus::operating);

                println("\033[32;1mExecuting task in ", timeUntil, " seconds\033[0m");
                return ScheduleReturnCode::operating;
            } else {
                // Wake up before not due to alarm, reschedule anyway
                power.scheduleNextAlarm(task.schedule - 8);  // 3 < x < 10
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

        for (auto v : task.valves) {
            switch (vm.valves[v].status) {
            case ValveStatus::unavailable: {
                KPStringBuilder<100> error("Valve ", v, " is not available");
                response["error"] = (char *) error;
                return;
            }
            case ValveStatus::sampled: {
                KPStringBuilder<100> error("Valve ", v, " has already been sampled");
                response["error"] = (char *) error;
                return;
            }
            case ValveStatus::operating: {
                KPStringBuilder<100> error("Valve ", v, " is operating");
                response["error"] = (char *) error;
                return;
            }
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
        pump.off();                    // Turn off motor
        shift.writeAllRegistersLow();  // Turn off all TPIC devices
        intake.off();

        tm.writeToDirectory();
        vm.writeToDirectory();
        power.shutdown();
        halt(TRACE, "Shutdown. This message should not be displayed. Check power module");
    }

    void invalidateTaskAndFreeUpValves(Task & task) {
        for (auto i = task.getValveOffsetStart(); i < task.getNumberOfValves(); i++) {
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