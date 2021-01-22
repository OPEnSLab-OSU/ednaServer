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
        setUpdateFreq(1000);
    }

public:
    double volume = 0;
    double lpm    = 0;

    void resetVolume() {
        volume = 0;
    }

    void startMeasurement() {
        attachInterrupt(digitalPinToInterrupt(A3), flowTick, FALLING);
    }

    void stopMeasurement() {
        detachInterrupt(digitalPinToInterrupt(A3));
    }

    SensorData read() override {
        if (flowUpdated) {
            flowUpdated = false;

            auto hz = 1000000.0 / double(flowIntervalMicros);
            lpm     = hz < 37 ? 0 : interpolate(hz, 37, 917, 0.110, 2.476);
            volume += lpm * (flowIntervalMicros / 60000000.0);
            println("Volume: ", volume, ", LPM: ", lpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }

        return {volume, lpm};
    }
};