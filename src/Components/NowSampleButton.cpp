#include "Components/NowSampleButton.hpp"

volatile unsigned long buttonInterruptStart = 0;
volatile bool buttonTriggered             = false;

void button_isr() {
    println("Button interrupt");
    if ((unsigned long) (millis() - buttonInterruptStart) < 1000) {
        println("Debounce detected!");
        return;
    }

    buttonTriggered    = true;
    buttonInterruptStart = millis();
}