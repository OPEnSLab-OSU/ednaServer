#pragma once
#include <Components/Sensor.hpp>
#include <SparkFun_MS5803_I2C.h>

class BaroSensor : public Sensor<float, float> {
private:
	MS5803 sensor;

public:
	BaroSensor(ms5803_addr addr) : sensor(addr) {}

	void begin() override {
		sensor.begin();
	}

	SensorData read() override {
		return {sensor.getPressure(ADC_4096), sensor.getTemperature(CELSIUS, ADC_4096)};
	}
};