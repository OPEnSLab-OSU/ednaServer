#include "Components/Power.hpp"

volatile unsigned long rtcInterruptStart = 0;
volatile bool alarmTriggered			 = false;

void isr() {
	if ((unsigned long) (millis() - rtcInterruptStart) < 1000) {
		return;
	}

	alarmTriggered	  = true;
	rtcInterruptStart = millis();
}
