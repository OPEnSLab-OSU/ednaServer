#include "Components/NowSampleButton.hpp"

volatile unsigned long buttonInterruptStart = 0;
volatile bool buttonTriggered             = false;
volatile int buttonFlag                    = 0;

void button_isr() {
    println(BLUE("BUTTON PRESS DETECTED"));
    if ((millis() - buttonInterruptStart) < 200) {
        print(RED("Debounce detected"));
        return;
    }

    buttonTriggered    = true;
    buttonInterruptStart = millis();
    buttonFlag = 1;
}