#include <Components/Power.hpp>

volatile unsigned long rtcInterruptStart    = 0;
volatile bool alarmTriggered                = false;
volatile int buttonFlag                     = 0;
volatile unsigned long buttonInterruptStart = 0;
volatile bool buttonTriggered               = 0;


void rtc_isr() {
    if (((unsigned long) (millis() - rtcInterruptStart) < 1000) && ((unsigned long) (millis() - buttonInterruptStart) < 1000)){
        return;
    }
	
	if(digitalRead(HardwarePins::BUTTON_PIN) == LOW){
		buttonTriggered    = true;
    	buttonInterruptStart = millis();
    	buttonFlag = 1;
	}

    alarmTriggered    = true;
    rtcInterruptStart = millis();
}
