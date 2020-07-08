#pragma once
#include <KPFoundation.hpp>
#include <Application/Constants.hpp>

class Pump : public KPComponent {
public:
	using KPComponent::KPComponent;
	const int control1;
	const int control2;

	Pump(const char * name, int control1, int control2)
		: KPComponent(name), control1(control1), control2(control2) {
		pinMode(control1, OUTPUT);
		pinMode(control2, OUTPUT);
		off();
	}

	void on(ValveDirection dir = ValveDirection::normal) {
		delay(20);
		analogWrite(control1, dir == ValveDirection::normal ? 255 : 0);
		analogWrite(control2, dir == ValveDirection::normal ? 0 : 255);
	}

	void off() {
		analogWrite(control1, 0);
		analogWrite(control2, 0);
		delay(20);
	}

	void pwm(float duty_cycle, ValveDirection dir = ValveDirection::normal) {
		uint8_t intensity = constrain(duty_cycle, 0, 1) * 255;
		analogWrite(dir == ValveDirection::normal ? control1 : control2, intensity);
		analogWrite(dir == ValveDirection::normal ? control2 : control1, 0);
	}
};