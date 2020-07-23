#pragma once
#include <KPFoundation.hpp>
#include <KPStateMachine.hpp>
#include <KPState.hpp>

#define STATE(x) constexpr const char * x = #x "_STATE"

template <typename T>
struct StateControllerConfigurator {
	using Config												 = T;
	virtual void configureStateController(Config & config) const = 0;
};

template <typename T>
struct StateControllerConfig {
	using Config	   = T;
	using Configurator = StateControllerConfigurator<Config>;
	Config config;

	void configure(const StateControllerConfigurator<Config> & configurator) {
		configurator.configureStateController(config);
	}

	void configure(const Config & config) {
		this->config = config;
	}
};

class StateController : public KPStateMachine {
public:
	StateController(const char * name) : KPStateMachine(name) {}

	virtual void begin() = 0;
	virtual void stop()	 = 0;
	virtual void idle()	 = 0;

	virtual void setup() override {
		println("StateMachine Setup ");
	}
};

template <typename T>
class StateControllerWithConfig : public StateController, public StateControllerConfig<T> {
public:
	using StateController::StateController;
	// using Configurator = StateControllerConfigurator<T>;
};