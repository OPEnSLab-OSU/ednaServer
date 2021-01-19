#pragma once
#include <Components/Sensor.hpp>

extern volatile unsigned long lastFlowTick;
extern volatile unsigned long flowIntervalMicros;
extern volatile bool flowUpdated;

void flowTick();

inline double interpolate(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

struct TurbineFlowSensorData {
    double volume;
    double lpm;
};

class TurbineFlowSensor : public Sensor<TurbineFlowSensorData> {
private:
    void begin() override {
        lastFlowTick = micros();
        pinMode(A3, INPUT);
        attachInterrupt(digitalPinToInterrupt(A3), flowTick, FALLING);
        setUpdateFreq(300);
    }

public:
    double volume = 0;
    double lpm    = 0;

    void resetVolume() {
        volume = 0;
    }

    SensorData read() override {
        if (flowUpdated) {
            flowUpdated = false;

            auto flowIntervalSecs = double(flowIntervalMicros) / 1000000.0;
            auto hz               = double(1) / flowIntervalSecs;
            lpm                   = hz < 37 ? 0 : interpolate(hz, 37, 917, 0.1, 2.5);
            volume += lpm * (flowIntervalSecs / 60.0);
            println(volume, lpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }

        return {volume, lpm};
    }
};