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

		// Setup serial communication
		Serial.begin(115200);
		while (!Serial) {
		};
		// delay(5000);

		addComponent(sm);
		addComponent(web);
		addComponent(fileLoader);
		addComponent(shift);
		addComponent(power);
		addComponent(pump);
		addComponent(scheduler);

		setupServer();
		web.begin();

		sm.addListener(&status);
		sm.registerState(StateIdle(), StateName::IDLE);
		sm.registerState(StateStop(), StateName::STOP);
		sm.registerState(StateFlush(), StateName::FLUSH);
		sm.registerState(StateSample(), StateName::SAMPLE);
		sm.registerState(StatePreserve(), StateName::PRESERVE);
		sm.registerState(StateDecon(), StateName::DECON);

		KPSerialInput::instance().addListener(this);

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
	}

	void update() override {
		KPController::update();
	}
};