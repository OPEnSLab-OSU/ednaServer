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
#include "KPFoundation.hpp"
#include "DS3232RTC.h"
#include "LowPower.h"
#include "Wire.h"
#include <SPI.h>
#include <Functional>

#include "Application/Constants.hpp"

//
// ────────────────────────────────────────────────────────────────────────────────────────── I ──────────
//   :::::: P O W E R   M A N A G E M E N T   W I T H   R T C : :  :   :    :     :        :          :
// ────────────────────────────────────────────────────────────────────────────────────────────────────
//
// DS3232RTC Library already know the address internally but we need this to check
// if RTC is connected
//
#define RTC_ADDR 0x68
extern volatile unsigned long rtcInterruptStart;
extern volatile bool alarmTriggered;
extern void rtc_isr();

class Power : public KPComponent {
public:
	DS3232RTC rtc;
	std::function<void()> interruptCallback;

	Power(const char * name)
		: KPComponent(name), rtc(false) {}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set interrupt callback
	// ────────────────────────────────────────────────────────────────────────────────
	void onInterrupt(std::function<void()> callbcak) {
		interruptCallback = callbcak;
	}

	void setup() override {
		// Initilize RTC I2C Bus
		waitForConnection();
		rtc.begin();

		// Reset RTC to a known state, clearing alarms, clear interrupts
		resetAlarms();
		rtc.squareWave(SQWAVE_NONE);

		// Register RTC as the external time provider for Time library
		setSyncProvider(rtc.get);
		setTime(rtc.get());

		// Print out the current time
		println("RTC startup time: ");
		printCurrentTime();

		// Register interrupt pin as active low
		pinMode(HardwarePins::RTC_INTERRUPT, INPUT_PULLUP);
		pinMode(HardwarePins::POWER_MODULE, OUTPUT);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Runtime update loop. Check if RTC has been triggered.
	// ────────────────────────────────────────────────────────────────────────────────
	void update() override {
		if (!alarmTriggered || !interruptCallback) {
			return;
		}

		// Check if the interrupt is comming from RTC
		if (rtc.alarm(1) || rtc.alarm(2)) {
			disarmAlarms();
			interruptCallback();
			alarmTriggered = false;
		}
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Wait for RTC to connect. This is done by checking if the return byte
	 *  from the RTC is -1. In two complement's signed interger, all bits high means
	 *  -1.Since, I2C for DS3231 is active low, this means that the RTC is not
	 *  connected
	 *  ──────────────────────────────────────────────────────────────────────────── */
	void waitForConnection() {
		Wire.begin();
		for (;; delay(5000)) {
			Wire.requestFrom(RTC_ADDR, 1, false);  // false: don't release I2C line
			if (Wire.read() == -1) {
				println("\n\033[31;1mRTC Not Connected\033[0m");
			} else {
				println("\n\033[32;1mRTC Connected\033[0m");
				break;
			}
		}

		Wire.end();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Signal the power module to cut power from the system
	// ────────────────────────────────────────────────────────────────────────────────
	void shutdown() {
		digitalWrite(HardwarePins::POWER_MODULE, HIGH);
		delay(20);
		digitalWrite(HardwarePins::POWER_MODULE, LOW);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set alarms registers to a known value and clear any prev alarms
	// ────────────────────────────────────────────────────────────────────────────────
	void resetAlarms() {
		rtc.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
		rtc.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
		disarmAlarms();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Clear previous alarms and disable interrupts
	// ────────────────────────────────────────────────────────────────────────────────
	void disarmAlarms() {
		rtc.alarm(ALARM_1);
		rtc.alarm(ALARM_2);
		rtc.alarmInterrupt(ALARM_1, false);
		rtc.alarmInterrupt(ALARM_2, false);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Bring the chip to the low power mode.
	// External interrupt is required to awake the device and resume operation
	// ────────────────────────────────────────────────────────────────────────────────
	void sleepForever() {
		println();
		println("Going to sleep...");
		for (int i = 3; i > 0; i--) {
			print(F("-> "));
			println(i);
			delay(333);
		}

		LowPower.standby();
		println();
		println("Just woke up due to interrupt!");
		printCurrentTime();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Put the chip into the low power state for specified number of seconds
	// ────────────────────────────────────────────────────────────────────────────────
	void sleepFor(unsigned long seconds) {
		setTimeout(seconds, true);
		sleepForever();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Schedule alarm for specified number of seconds from now.
	// @param usingInterrupt:
	//     A flag controlling whether to trigger the interrupt service routine
	// ────────────────────────────────────────────────────────────────────────────────
	void setTimeout(unsigned long seconds, bool usingInterrupt) {
		TimeElements future;
		breakTime(rtc.get() + seconds, future);
		disarmAlarms();
		rtc.setAlarm(ALM1_MATCH_MINUTES, future.Second, future.Minute, future.Hour, 0);
		if (usingInterrupt) {
			attachInterrupt(digitalPinToInterrupt(HardwarePins::RTC_INTERRUPT), rtc_isr, FALLING);
			rtc.alarmInterrupt(1, true);
		}
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Schedule RTC alarm given TimeElements
	// @param future:
	//     Must be in the future, otherwise this method does nothing
	// ────────────────────────────────────────────────────────────────────────────────
	void scheduleNextAlarm(TimeElements future) {
		unsigned long utc = makeTime(future);
		scheduleNextAlarm(utc);
	}

	void scheduleNextAlarm(unsigned long utc) {
		unsigned long timestamp = now();
		if (utc < timestamp) {
			return;
		}

		println("Alarm triggering in: ", utc - timestamp, " seconds");
		setTimeout(utc - timestamp, true);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set RTC time and internal timer of the Time library
	// ────────────────────────────────────────────────────────────────────────────────
	void set(unsigned long seconds) {
		println("Setting RTC Time...");
		printTime(seconds);

		setTime(seconds);  // Set time in Time library
		rtc.set(seconds);  // Set time for RTC
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Print time to console formatted as YYYY.MM.DD @ hh:mm:ss
	// ────────────────────────────────────────────────────────────────────────────────
	void printTime(unsigned long seconds) {
		char message[64];
		sprintf(message,
			"%u.%u.%u @ %02u:%02u:%02u",
			year(seconds),
			month(seconds),
			day(seconds),
			hour(seconds),
			minute(seconds),
			second(seconds));
		println(message);
	}

	void printTime(unsigned long seconds, int offset) {
		printTime(seconds + (offset * 60 * 60));
	}

	void printCurrentTime(int offset = 0) {
		print("Current Time: ");
		printTime(rtc.get(), offset);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Convert compiled timestrings to seconds since 1 Jan 1970
	// ────────────────────────────────────────────────────────────────────────────────
	time_t compileTime(int offsetMinutes = 0) {
		const time_t FUDGE	  = 10;	 //Fudge factor to allow for upload time
		const char * compDate = __DATE__;
		const char * compTime = __TIME__;
		const char * months	  = "JanFebMarAprMayJunJulAugSepOctNovDec";
		char * m;

		// Get month from compiled date
		char compMon[4]{0};
		strncpy(compMon, compDate, 3);
		m = strstr(months, compMon);

		TimeElements tm;
		tm.Month  = ((m - months) / 3 + 1);
		tm.Day	  = atoi(compDate + 4);
		tm.Year	  = atoi(compDate + 7) - 1970;
		tm.Hour	  = atoi(compTime);
		tm.Minute = atoi(compTime + 3);
		tm.Second = atoi(compTime + 6);

		time_t t = makeTime(tm);
		if (offsetMinutes) {
			t += offsetMinutes * 60;
			breakTime(t, tm);
		}

		printTime(t);
		return t + FUDGE;  //Add fudge factor to allow for compile time
	}
};
