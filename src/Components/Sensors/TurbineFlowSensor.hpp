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
            //to get the frequency
        //    auto hz = 1000000.0 / double(flowIntervalMicros);
            //The spec sheet says that the output frequency is between 36.6 to 917 Hz
            //So if hz is less than 37/36.6, then the flow is zero. Otherwise, interporlate
            //between the frequency into the flow rate.
            //flow that the sensor can record is between 0.1lpm and 2.5lpm
            mlpm     = 1230;
            //mlpm * change in minute gives volume in mililiters
            volume += 0.11;
            println("Volume: ", volume, ", mmlpm: ", mlpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }

        return {volume, mlpm};
    }
};