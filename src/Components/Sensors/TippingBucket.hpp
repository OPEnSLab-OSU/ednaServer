#pragma once
#include <Components/Sensor.hpp>
#include <Application/Constants.hpp>


extern volatile unsigned long lastTipTime;
extern volatile unsigned int tipCount;
extern volatile bool tipUpdated;
extern volatile unsigned long  tipIntervalMicros;

void tippingBucketInterrupt();

struct TippingBucketSensorData {
    double volume; //volume in liters
    double tips; //tips
};

class TippingBucket : public Sensor<TippingBucketSensorData> {
private:
    double volumePerTip; 
    void begin() override {
        tipCount = 0;
        pinMode(HardwarePins:: ANALOG_SENSOR_1, INPUT);
        setUpdateFreq(1000);
    }

public:
    double volume = 0;
    double mlpm    = 0;

    void resetVolume() {
        volume = 0;
    }

    void startMeasurement() {
        attachInterrupt(digitalPinToInterrupt(HardwarePins:: ANALOG_SENSOR_1), tippingBucketInterrupt, FALLING);
    }

    void stopMeasurement() {
        detachInterrupt(digitalPinToInterrupt(HardwarePins:: ANALOG_SENSOR_1));
    }

    SensorData read() override {
        if(tipUpdated) {
            tipUpdated = false;
            println("inside tipping bucket read");
            auto tipInterval = double(tipIntervalMicros)/60000000;
            mlpm = 1/tipInterval;
            volumePerTip = 1; //1 ml per tip
            volume+= volumePerTip;

            println("Volume: ", volume, ", mLpm: ", mlpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }   
        return {volume};
}
};