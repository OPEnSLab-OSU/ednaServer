#pragma once
#include "KPFoundation.hpp"
#include "DS3232RTC.h"
#include "LowPower.h"
#include "Wire.h"
#include <SPI.h>
#include <Functional>

#include "Application/Constants.hpp"

#define RTC_ADDR 0x68  // Library already has the address but this is needed for checking

//===========================================================
// [+_+]
//===========================================================
using ulong = unsigned long;

extern volatile ulong rtcInterruptStart;
extern volatile bool alarmTriggered;
extern void isr();

class Power : public KPComponent {
public:
	DS3232RTC timeKeeper;
	std::function<void()> interruptCallback;

	Power(const char * name)
		: KPComponent(name), timeKeeper(false) {
		pinMode(Power_Module_Pin, OUTPUT);
	}

	void onInterrupt(std::function<void()> callbcak) {
		interruptCallback = callbcak;
	}

	void setup() override {
		// Check if RTC is connected
		waitForConnection();

		// Initilize RTC I2C Bus
		timeKeeper.begin();

		// Reset RTC to a known state, clearing alarms, clear interrupts
		resetAlarms();
		timeKeeper.squareWave(SQWAVE_NONE);

		// Register RTC as the external time provider for Time library
		setSyncProvider(timeKeeper.get);
		setTime(timeKeeper.get());

		// Print out the current time
		println("Initial Startup: ");
		printCurrentTime();

		// Register interrupt pin as active low
		pinMode(RTC_Interrupt_Pin, INPUT_PULLUP);
	}

	void update() override {
		if (alarmTriggered && interruptCallback) {
			alarmTriggered = false;
			disarmAlarms();
			interruptCallback();
		}
	}

	//
	// ─── WAIT FOR RTC CONNECTION THIS IS DONE BY ASKING THE RTC TO RETURN A BYTE ────────
	//

	void waitForConnection() {
		Wire.begin();
		Wire.requestFrom(RTC_ADDR, 1, false);  // false: don't release I2C line

		for (;; delay(2000)) {
			// This means that all bits are high. I2C for DS3231 is active low.
			if (Wire.read() != -1) {
				println("\n\033[32;1m[^_^] RTC Connected\033[0m");
				break;
			} else {
				println("\n\033[32;1m[-_-] RTC Not Connected\033[0m");
			}
		}
	}

	void shutdown() {
		digitalWrite(Power_Module_Pin, HIGH);
		delay(20);
		digitalWrite(Power_Module_Pin, LOW);
	}

	//===========================================================
	// [+_+] Set alarms registers to a known value and clear any prev alarms
	//===========================================================
	void resetAlarms() {
		timeKeeper.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
		timeKeeper.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
		disarmAlarms();
	}

	//===========================================================
	// [+_+] Clear previous alarms and disable interrupts
	//===========================================================
	void disarmAlarms() {
		timeKeeper.alarm(ALARM_1);
		timeKeeper.alarm(ALARM_2);
		timeKeeper.alarmInterrupt(ALARM_1, false);
		timeKeeper.alarmInterrupt(ALARM_2, false);
	}

	//===========================================================
	// [+_+] Bring the chip to the low power mode.
	// External interrupt is required to awake the device and resume operation
	//===========================================================
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

	//===========================================================
	// [+_+] Put the chip into the low power state for specified number of seconds
	//===========================================================
	void sleepFor(ulong seconds) {
		setTimeout(seconds, true);
		sleepForever();
	}

	//===========================================================
	// [+_+] Schedule alarm for specified number of seconds from now.
	// @param bool usingInterrupt: A flag controlling whether to trigger the interrupt service routine
	//===========================================================
	void setTimeout(ulong seconds, bool usingInterrupt) {
		TimeElements future;
		breakTime(timeKeeper.get() + seconds, future);
		disarmAlarms();
		timeKeeper.setAlarm(ALM1_MATCH_MINUTES, future.Second, future.Minute, future.Hour, 0);
		if (usingInterrupt) {
			attachInterrupt(digitalPinToInterrupt(RTC_Interrupt_Pin), isr, FALLING);
			timeKeeper.alarmInterrupt(1, true);
		}
	}

	//===========================================================
	// [+_+] Schedule RTC alarm given TimeElements
	// @param TimeElements future: must be in the future, otherwise this method does nothing
	//===========================================================
	void scheduleNextAlarm(TimeElements future) {
		ulong utc = makeTime(future);
		scheduleNextAlarm(utc);
	}

	void scheduleNextAlarm(ulong utc) {
		ulong timestamp = now();
		if (utc < timestamp) {
			return;
		}

		println("Alarm triggering in: ", utc - timestamp, " seconds");
		setTimeout(utc - timestamp, true);
	}

	//===========================================================
	// [+_+] Set RTC time and internal timer of the Time library
	//===========================================================
	void set(ulong seconds) {
		print("Setting RTC Time...");
		printTime(seconds);

		setTime(seconds);
		timeKeeper.set(seconds);
	}

	//===========================================================
	// [+_+] Print time to console formatted as YYYY.MM.DD : hh.mm.ss
	//===========================================================
	void printTime(ulong seconds) {
		char message[64]{};
		sprintf(message, "%u.%u.%u : %u.%u.%u", year(seconds), month(seconds),
			day(seconds), hour(seconds), minute(seconds), second(seconds));
		println(message);
	}

	void printTime(ulong seconds, int offset) {
		printTime(seconds + (offset * 60 * 60));
	}

	void printCurrentTime(int offset = 0) {
		print("Current Time: ");
		printTime(timeKeeper.get(), offset);
	}

	//===========================================================
	// [+_+] Convert compiled timestrings to seconds since 1 Jan 1970
	//===========================================================
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

		println("date=", compDate);
		println("time=", tm.Hour, "h ", tm.Minute, "m ", tm.Second, "s");
		return t + FUDGE;  //Add fudge factor to allow for compile time
	}
};
