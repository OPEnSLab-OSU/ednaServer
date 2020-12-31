#pragma once
#include <Components/Sensor.hpp>

extern volatile unsigned long lastFlowTick;
extern volatile unsigned long flowIntervalMicros;
extern volatile bool flowUpdated;

void flowTick();

struct TurbineFlowSensorData {
	double totalVolume;
	double lpm;
};

struct TurbineFlowSensor : public Sensor<TurbineFlowSensorData> {
	double totalVolume = 0;
	double lpm		   = 0;

	void begin() override {
		lastFlowTick = micros();
		pinMode(A4, INPUT);
		attachInterrupt(digitalPinToInterrupt(A4), flowTick, FALLING);
		setUpdateFreq(300);
	}

	SensorData read() override {
		if (flowUpdated) {
			flowUpdated = false;

			auto flowIntervalSecs = double(flowIntervalMicros) / 1000000.0;
			int hz				  = double(1) / flowIntervalSecs;
			lpm					  = hz < 37 ? 0 : map(hz, 37, 917, 1, 25) / 10.0;
			totalVolume += lpm * (flowIntervalSecs / 60.0);
		} else {
			setErrorCode(ErrorCode::notReady);
		}

		return {totalVolume, lpm};
	}
};