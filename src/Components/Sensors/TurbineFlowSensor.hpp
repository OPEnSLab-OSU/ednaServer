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
    double mlpm;
};

class TurbineFlowSensor : public Sensor<TurbineFlowSensorData> {
private:
    void begin() override {
        lastFlowTick = micros();
        pinMode(A3, INPUT);
        //sensor is updated once a second
        setUpdateFreq(1000);
    }

public:
    double volume = 0;
    double mlpm    = 0;

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
            //micro is 1e-6, so we divide the micros in a second with the flow interval
            //flowInterval (minutes) = x ms * (1s/1e*6ms) * (1m/60s)
            auto flowInterval = double(flowIntervalMicros)/60000000;
            //Sensor Pulse Constant: 0.11 ml
            mlpm    = 0.11/flowInterval;
            //mlpm * change in minute gives volume in mililiters
            volume += 0.11;
            println("Volume: ", volume, ", mmlpm: ", mlpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }

        return {volume, mlpm};
    }
};