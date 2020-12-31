#include <Components/Sensors/TurbineFlowSensor.hpp>

volatile unsigned long lastFlowTick;
volatile unsigned long flowIntervalMicros;
volatile bool flowUpdated;

void flowTick() {
	flowIntervalMicros = micros() - lastFlowTick;
	lastFlowTick	   = micros();
	flowUpdated		   = true;
}