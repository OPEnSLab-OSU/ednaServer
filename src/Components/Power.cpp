#include "Components/Power.hpp"

volatile unsigned long rtcInterruptStart = 0;
volatile bool alarmTriggered             = false;

void rtc_isr() {
    if (((unsigned long) (millis() - rtcInterruptStart) < 1000) && ((unsigned long) (millis() - buttonInterruptStart) < 200)){
        return;
    }
	
	if(digitalRead(HardwarePins::BUTTON_PIN){
		buttonTriggered    = true;
    	buttonInterruptStart = millis();
    	buttonFlag = 1;
	}

    alarmTriggered    = true;
    rtcInterruptStart = millis();
}
