#include "Components/NowSampleButton.hpp"

volatile unsigned long buttonInterruptStart = 0;
volatile bool buttonTriggered             = false;
volatile int buttonFlag                    = 0;

void button_isr() {
    if ((unsigned long) (millis() - buttonInterruptStart) < 200) {
        return;
    }

    buttonTriggered    = true;
    buttonInterruptStart = millis();
    buttonFlag = 1;
}