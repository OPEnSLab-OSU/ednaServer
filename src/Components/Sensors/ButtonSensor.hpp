#pragma once
#include <Components/Sensor.hpp>
#include <Arduino.h>
#include <Wire.h>

typedef union {
    struct 
    {
        bool eventAvailable : 1; //This is bit 0. User mutable, gets set to 1 when a new event occurs. User is expected to write 0 to clear the flag.
        bool hasBeenClicked : 1; //Defaults to zero on POR. Gets set to one when the button gets clicked. Must be cleared by the user.
        bool isPressed : 1;      //Gets set to one if button is pushed.
        bool : 5;
    };
    uint8_t byteWrapped;
} status;

class ButtonSensor : public Sensor<float, float> {
private:
    const int ADDR;
    void begin() override {
        setUpdateFreq(3);
        Wire.begin();
    };

public:
    ButtonSensor(int addr) : ADDR(addr) {}

    SensorData read() override {
        Wire.beginTransmission(ADDR);
        Wire.write(0x03); //CHANGE LATER
        Wire.endTransmission();
        if(Wire.requestFrom(ADDR, static_cast<uint8_t>(1)) != 0){
            status s;
            s.byteWrapped = Wire.read();
            return {(int)s.isPressed, 1};
        }
    }
};
