#pragma once
#include <Components/Sensor.hpp>
#include <SSC.h>

class PressureSensor : public Sensor<float, float> {
private:
    SSC sensor;

    void begin() override {
        setUpdateFreq(3);
        sensor.setMinRaw(1638);
        sensor.setMaxRaw(14745);
        sensor.setMinPressure(0);
        sensor.setMaxPressure(30);
        sensor.start();
    };

public:
    PressureSensor(int addr) : sensor(addr) {}

    SensorData read() override {
        sensor.update();
        return {sensor.pressure(), sensor.temperature()};
    }
};
