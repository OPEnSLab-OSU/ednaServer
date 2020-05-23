#pragma once
#define __k_str constexpr const char *
#define __k_int constexpr int

namespace HardwarePins {
	__k_int POWER_MODULE	  = A0;
	__k_int RTC_INTERRUPT	  = 12;	 // A1 is eDNA, 12 for HYPNOS
	__k_int BATTERY_VOLTAGE	  = A2;
	__k_int ANALOG_SENSOR_1	  = A3;
	__k_int ANALOG_SENSOR_2	  = A4;
	__k_int SHUTDOWN_OVERRIDE = A5;
	__k_int MOTOR_FORWARD	  = 5;
	__k_int MOTOR_REVERSE	  = 6;
	__k_int SHFT_REG_LATCH	  = 9;
	__k_int SD_CARD			  = 10;
	__k_int SHFT_REG_CLOCK	  = 11;
	__k_int SHFT_REG_DATA	  = 12;
	__k_int BUTTON_PIN		  = 13;
};	// namespace HardwarePins

namespace ProgramSettings {
	__k_str CONFIG_FILE_PATH		  = "config.js";
	__k_int SD_FILE_NAME_LENGTH		  = 13;
	__k_int CONFIG_JSON_BUFFER_SIZE	  = 500;
	__k_int STATUS_JSON_BUFFER_SIZE	  = 800;
	__k_int TASK_JSON_BUFFER_SIZE	  = 500;
	__k_int TASKREF_JSON_BUFFER_SIZE  = 50;
	__k_int MAX_VALVES				  = 24;
	__k_int VALVE_JSON_BUFFER_SIZE	  = 500;
	__k_int VALVEREF_JSON_BUFFER_SIZE = 50;
	__k_int VALVE_GROUP_LENGTH		  = 25;

};	// namespace ProgramSettings

namespace ConfigKeys {
	__k_str VALVES_FREE		  = "freeValves";
	__k_str VALVE_UPPER_BOUND = "valveUpperBound";
	__k_str FILE_LOG		  = "logFile";
	__k_str FILE_STATUS		  = "statusFile";
	__k_str FOLDER_TASK		  = "taskFolder";
	__k_str FOLDER_VALVE	  = "valveFolder";
}  // namespace ConfigKeys

namespace TaskSettings {
	__k_int NAME_LENGTH	 = 25;
	__k_int GROUP_LENGTH = 25;
	__k_int NOTES_LENGTH = 128;
};	// namespace TaskSettings

namespace JsonKeys {
	__k_str STATE_ID			  = "stateId";
	__k_str STATE_CURRENT_NAME	  = "stateCurrentName";
	__k_str STATE_NAME			  = "stateName";
	__k_str STATE_INDEX			  = "stateIndex";
	__k_str VALVES				  = "valves";
	__k_str VALVES_COUNT		  = "valvesCount";
	__k_str VALVE_ID			  = "valveId";
	__k_str VALVE_GROUP			  = "valveGroup";
	__k_str VALVE_SCHEDULE		  = "valveSchedule";
	__k_str VALVE_CURRENT		  = "valveCurrent";
	__k_str VALVE_STATUS		  = "valveStatus";
	__k_str VALVE_FLUSH_TIME	  = "valveFlushTime";
	__k_str VALVE_FLUSH_VOLUME	  = "valveFlushVolume";
	__k_str VALVE_SAMPLE_TIME	  = "valveSampleTime";
	__k_str VALVE_SAMPLE_PRESSURE = "valveSamplePressure";
	__k_str VALVE_SAMPLE_VOLUME	  = "valveSampleVolume";
	__k_str VALVE_DRY_TIME		  = "valveDryTime";
	__k_str VALVE_PRESERVE_TIME	  = "valvePreserveTime";
	__k_str TASK_CURRENT_NAME	  = "taskCurrentName";
	__k_str TASK_NAME			  = "name";
	__k_str TASK_NEW_NAME		  = "newName";
	__k_str TIME_UTC			  = "timeUTC";
	__k_str SENSOR_PRESSURE		  = "pressure";
	__k_str SENSOR_TEMP			  = "temperature";
	__k_str SENSOR_BARO			  = "barometric";
	__k_str SENSOR_VOLUME		  = "waterVolume";
	__k_str SENSOR_FLOW			  = "waterFlow";
	__k_str SENSOR_DEPTH		  = "waterDepth";
};	// namespace JsonKeys

#undef __k_str
#undef __k_int
