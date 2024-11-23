#include <Components/Sensors/TippingBucket.hpp>

volatile unsigned long lastTipTime = 0;
volatile unsigned int tipCount = 0;
volatile bool tipUpdated = false;

void tippingBucketInterrupt() {
    lastTipTime = micros(); // Update the time of the tip (optional for rate calculation)
    tipCount++;             // Increment tip count
    tipUpdated = true;      // Flag for new data
}