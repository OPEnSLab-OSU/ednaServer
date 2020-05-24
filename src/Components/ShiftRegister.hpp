/**
 * Copyright (c) 2020 Kawin Pechetratanapanit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include <KPFoundation.hpp>
#include <SPI.h>

class ShiftRegister : public KPComponent {
public:
	const int capacityPerRegister = 8;
	const int capacity;
	const int registersCount;

	int dataPin	 = 0;
	int clockPin = 0;
	int latchPin = 0;

	int8_t * registers;
	BitOrder bitOrder = MSBFIRST;

public:
	ShiftRegister(const char * name, int capacity, int data, int clock, int latch)
		: KPComponent(name),
		  capacity(capacity),
		  registersCount(capacity / capacityPerRegister) {
		registers = new int8_t[registersCount]();
		setRegisterPins(data, clock, latch);
	}

	void setup() override {
		writeAllRegistersLow();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Helper converting 1D pin index to register index
	// ────────────────────────────────────────────────────────────────────────────────
	int toRegisterIndex(int pinNumber) const {
		return pinNumber / capacityPerRegister;
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Helper converting 1D pin index to pin index
	// ────────────────────────────────────────────────────────────────────────────────
	int toPinIndex(int pinNumber) const {
		return pinNumber % capacityPerRegister;
	}

	std::pair<int, int> toRegisterAndPinIndices(int pinNumber) const {
		return {toRegisterIndex(pinNumber), toPinIndex(pinNumber)};
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set the hardware pins of the shift registers. The registers should be hooked up
	// in daisy chain mode.
	// ────────────────────────────────────────────────────────────────────────────────
	void setRegisterPins(int data, int clock, int latch) {
		dataPin	 = data;
		clockPin = clock;
		latchPin = latch;
		pinMode(dataPin, OUTPUT);
		pinMode(clockPin, OUTPUT);
		pinMode(latchPin, OUTPUT);
	}

	void setAllRegistersLow() {
		for (int i = 0; i < registersCount; i++) {
			registers[i] = 0;
		}
	}

	void setAllRegistersHigh() {
		for (int i = 0; i < registersCount; i++) {
			registers[i] = 0xFF;
		}
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set individual pin of the register high/low given the register index
	// ────────────────────────────────────────────────────────────────────────────────s
	void setRegister(int registerIndex, int pinIndex, bool signal) {
		if (registerIndex >= registersCount || pinIndex >= capacityPerRegister) {
			return;
		}

		if (signal) {
			registers[registerIndex] |= (1 << pinIndex);
		} else {
			registers[registerIndex] &= ~(1 << pinIndex);
		}
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set invidual pin of the register high/low
	// ────────────────────────────────────────────────────────────────────────────────
	void setPin(int index, bool signal) {
		if (index < 0 || index >= capacity) {
			return;
		}

		setRegister(toRegisterIndex(index), toPinIndex(index), signal);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Shiftout each byte in the register array in reverse order
	// ────────────────────────────────────────────────────────────────────────────────
	void write() {
		digitalWrite(latchPin, LOW);
		for (int i = registersCount - 1; i >= 0; i--) {
			shiftOut(dataPin, clockPin, bitOrder, registers[i]);
		}
		digitalWrite(latchPin, HIGH);
	}

	void writePin(int index, bool signal) {
		setPin(index, signal);
		write();
	}

	void writeRegister(int registerIndex, int pinIndex, bool signal) {
		setRegister(registerIndex, pinIndex, signal);
		write();
	}

	void writeAllRegistersLow() {
		setAllRegistersLow();
		write();
	}

	void writeAllRegistersHigh() {
		setAllRegistersHigh();
		write();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Turn on a single pin where the rest is low
	// One-hot: https://en.wikipedia.org/wiki/One-hot
	// ────────────────────────────────────────────────────────────────────────────────
	void writeOneHot(int pinNumber) {
		if (toRegisterIndex(pinNumber) >= registersCount) {
			return;
		}

		setAllRegistersLow();
		setPin(pinNumber, HIGH);
		write();
	}

private:
	// ────────────────────────────────────────────────────────────────────────────────
	// Convenient methods for working with latch valve
	// ────────────────────────────────────────────────────────────────────────────────
	void writeLatch(bool controlPin) {
		setPin(controlPin, HIGH);
		write();
		delay(80);
		setPin(controlPin, LOW);
		write();
	}

public:
	void writeLatchIn() {
		writeLatch(0);
	}

	void writeLatchOut() {
		writeLatch(1);
	}
};