#pragma once
#include <Components/Sensor.hpp>

extern volatile unsigned long lastTipTime;
extern volatile unsigned int tipCount;
extern volatile bool tipUpdated;

void tippingBucketInterupt();

inline double interpolate(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

struct TippingBucketSensorData {
    double volume; //volume in liters
    double tips; //tips
};

class TippingBucket : public Sensor<TippingBucketSensorData> {
private:
    double volumePerTip; 
    void begin() override {
        tipCount = 0;
        pinMode(A4, INPUT);
        setUpdateFreq(1000);
    }

public:
    double volume = 0;
    double lpm    = 0;

    void resetVolume() {
        volume = 0;
    }
    double calculateVolume() {
        return tipCount * volumePerTip; //volume in liters
    }
    
    void startMeasurement() {
        attachInterrupt(digitalPinToInterrupt(A4), tippingBucketInterupt, FALLING);
    }

    void stopMeasurement() {
        detachInterrupt(digitalPinToInterrupt(A4));
    }

    SensorData read() override {
        if(tipUpdated) {
            tipUpdated = false;
            
            volumePerTip = 0.01; //0.01 liters per tip
            volume = calculateVolume();
            println("Volume: ", volume, ", LPM: ", lpm);
        } else {
            setErrorCode(ErrorCode::notReady);
        }   
        return {calculateVolume(), tipCount};
};