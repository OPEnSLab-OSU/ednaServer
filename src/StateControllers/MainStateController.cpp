#include <StateControllers/MainStateController.hpp>
#include <Application/App.hpp>

void Main::Idle::enter(KPStateMachine & sm) {
    auto & app = *static_cast<App *>(sm.controller);
    println(app.scheduleNextActiveTask().description());
};

void Main::Stop::enter(KPStateMachine & sm) {
    auto & app = *static_cast<App *>(sm.controller);
    app.pump.off();
    app.shift.writeAllRegistersLow();
    app.intake.off();

    app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
    app.vm.writeToDirectory();

    auto currentTaskId = app.currentTaskId;
    if(app.tm.advanceTask(currentTaskId)){
        println(BLUE("Setting now sample button to be pressed again"));
        app.power.setSampleButton();
    };
    app.tm.writeToDirectory();

    if(app.ntm.advanceTask()){
        println(BLUE("Setting now sample button to be pressed again"));
        app.power.setSampleButton();
    }
    app.ntm.writeToDirectory();

    app.currentTaskId       = 0;
    app.status.currentValve = -1;
    sm.next();
}
