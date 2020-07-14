#pragma once
#include <KPFoundation.hpp>
#include <KPSubject.hpp>
#include <Components/SensorArrayObserver.hpp>

#include <SSC.h>
#include <SparkFun_MS5803_I2C.h>

#define PSAddr 0x08
#define FSAddr 0x07
#define BSAddr 0x77
#define DSAddr 0x76

inline bool checkForConnection(unsigned char addr) {
	Wire.begin();
	Wire.requestFrom(addr, 1);
	return Wire.read() != -1;
}

// The Curiously Recurring Template Pattern (CRTP) allows us to build functionalities
// without dynamic dispatch.
// See: https://www.fluentcpp.com/2017/05/12/curiously-recurring-template-pattern/

template <typename T>
class Crtp {
protected:
	T & underlying() {
		return static_cast<T &>(*this);
	}
	T const & underlying() const {
		return static_cast<T const &>(*this);
	}
};

template <typename SensorType>
class I2CSensor : public Crtp<SensorType> {
private:
	friend SensorType;
	I2CSensor() {}

public:
	float updateFreqHz				  = 2;
	unsigned long timeSinceLastUpdate = 0;
	unsigned char addr				  = 0;
	unsigned char enabled			  = false;
	unsigned char updated			  = false;

	void updateSensor() {
		if (!enabled) {
			return;
		}

		if ((millis() - timeSinceLastUpdate) >= 1000 / updateFreqHz) {
			this->underlying().update();
			updated				= true;
			timeSinceLastUpdate = millis();
		}
	}

	bool didUpdate() {
		if (updated) {
			updated = false;
			return true;
		} else {
			return false;
		}
	}
};

class PressureSensor : public I2CSensor<PressureSensor> {
	SSC sensor;

public:
	PressureSensor(PressureSensor & rhs) = delete;
	PressureSensor(unsigned char addr) : sensor{addr} {
		this->addr = addr;
	}

	void setup() {
		enabled = checkForConnection(addr);
		if (enabled) {
			sensor.setMinRaw(1638);
			sensor.setMaxRaw(14745);
			sensor.setMinPressure(0);
			sensor.setMaxPressure(30);
			sensor.start();
		}
	};

	void update() {
		sensor.update();
	}

public:
	std::pair<float, float> getValue() {
		return {sensor.pressure(), sensor.temperature()};
	}
};

class BaroSensor : public I2CSensor<BaroSensor> {
	MS5803 sensor;

public:
	BaroSensor(ms5803_addr addr) : sensor(addr) {
		this->addr = addr;
	}

	void setup() {
		enabled = checkForConnection(addr);
		if (enabled) {
			sensor.begin();
		}
	}

	void update() {}

	std::pair<float, float> getValue() {
		return {sensor.getPressure(ADC_4096), sensor.getTemperature(CELSIUS, ADC_4096)};
	}
};

class SensorArray : public KPComponent, public KPSubject<SensorArrayObserver> {
public:
	using KPComponent::KPComponent;
	PressureSensor ps{PSAddr};
	BaroSensor baro1{ADDRESS_HIGH};
	BaroSensor baro2{ADDRESS_LOW};

	void setup() override {
		ps.setup();
		ps.updateFreqHz = 2;
		println(ps.enabled ? GREEN("Pressure sensor connected")
						   : RED("Pressure sensor not connected"));
	}

	void updatePs() {
		if (!ps.enabled) {
			return;
		}

		if (ps.didUpdate()) {
			updateObservers(&SensorArrayObserver::pressureSensorDidUpdate, ps.getValue());
		}

		ps.updateSensor();
	}

	void updateBaro1() {
		if (!baro1.enabled) {
			return;
		}

		if (baro1.didUpdate()) {
			updateObservers(&SensorArrayObserver::baro1DidUpdate, baro1.getValue());
		}

		baro1.updateSensor();
	}

	void updateBaro2() {
		if (!baro2.enabled) {
			return;
		}

		if (baro2.didUpdate()) {
			updateObservers(&SensorArrayObserver::baro2DidUpdate, baro2.getValue());
		}

		baro2.updateSensor();
	}

	void update() override {
		updatePs();
		updateBaro1();
		updateBaro2();
	}
};