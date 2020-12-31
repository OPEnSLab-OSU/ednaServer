#pragma once
#include <Components/Sensor.hpp>
#include <SparkFun_MS5803_I2C.h>

class BaroSensor : public Sensor<float, float> {
private:
    MS5803 sensor;

    void begin() override {
        setUpdateFreq(3);
        sensor.begin();
    }

public:
    BaroSensor(ms5803_addr addr) : sensor(addr) {}

    SensorData read() override {
        return {sensor.getPressure(ADC_4096), sensor.getTemperature(CELSIUS, ADC_4096)};
    }
};