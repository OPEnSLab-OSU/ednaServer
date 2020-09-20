#pragma once
#include <Components/Sensor.hpp>
#include <SSC.h>

class PressureSensor : public Sensor<float, float> {
	SSC sensor;

public:
	PressureSensor(int addr) : sensor(addr) {}

	void begin() override {
		sensor.setMinRaw(1638);
		sensor.setMaxRaw(14745);
		sensor.setMinPressure(0);
		sensor.setMaxPressure(30);
		sensor.start();
	};

public:
	SensorData read() override {
		sensor.update();
		return {sensor.pressure(), sensor.temperature()};
	}
};
