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
        //sensor is updated once a second
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
            //micro is 1e-6, so we divide the micros in a second with the flow interval
            //to get the frequency
            auto hz = 1000000.0 / double(flowIntervalMicros);
            //The spec sheet says that the output frequency is between 36.6 to 917 Hz
            //So if hz is less than 37/36.6, then the flow is zero. Otherwise, interporlate
            //between the frequency into the flow rate.
            //flow that the sensor can record is between 0.1LPM and 2.5LPM
            lpm     = hz < 37 ? 0 : interpolate(hz, 37, 917, 0.110, 2.476);
            //LPM * change in minute gives volume in liters
            volume += lpm * (flowIntervalMicros / 60000000.0);
            println("Volume: ", volume, ", LPM: ", lpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }

        return {volume, lpm};
    }
};