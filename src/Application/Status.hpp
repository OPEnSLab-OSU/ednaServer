#pragma once
#include <KPFoundation.hpp>
#include <Application/Config.hpp>
#include <array>

#include <Utilities/JsonFileLoader.hpp>
#include <Valve/ValveStatus.hpp>
#include <Valve/ValveObserver.hpp>

class Status : public JsonDecodable,
			   public JsonEncodable,
			   public Printable,
			   public KPStateMachineObserver,
			   public ValveObserver {
public:
	std::vector<int> valves;
	int currentValve = -1;

	float pressure	  = 0;
	float temperature = 0;
	float barometric  = 0;

	float waterVolume = 0;
	float waterDepth  = 0;
	float waterFlow	  = 0;

	const char * currentStateName = nullptr;
	const char * currentTaskName  = nullptr;

	bool isFull			 = false;
	bool preventShutdown = false;

	Status()			   = default;
	Status(const Status &) = delete;
	Status & operator=(const Status &) = delete;

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Initialize status from user config
	 * 
	 * @param config Config object containing meta data of the system
	**/
	void init(Config & config) {
		valves.resize(config.numberOfValves);
		memcpy(valves.data(), config.valves, sizeof(int) * config.numberOfValves);
	}

private:
	void valveArrayDidUpdate(const std::vector<Valve> & valves) override {
		currentValve = -1;
		for (const Valve & v : valves) {
			if (v.status == ValveStatus::operating) {
				currentValve = v.id;
			}

			this->valves[v.id] = v.status;
		}
	}

	void valveDidUpdate(const Valve & valve) override {
		valves[valve.id] = valve.status;
		if (valve.status == ValveStatus::operating) {
			currentValve = valve.id;
		}
	}

	void stateDidBegin(const KPState * current) override {
		currentStateName = current->getName();
	}

public:
	// ────────────────────────────────────────────────────────────────────────────────
	// Override_Mode_Pin is connected to an external switch which is active low.
	// Checking <= 100 to accomodate for noisy signal.
	// ────────────────────────────────────────────────────────────────────────────────
	static bool isProgrammingMode() {
		return analogRead(HardwarePins::SHUTDOWN_OVERRIDE) <= 100;
	}

#pragma region JSONDECODABLE
	static const char * decoderName() {
		return "Status";
	}

	static constexpr size_t decoderSize() {
		return ProgramSettings::STATUS_JSON_BUFFER_SIZE;
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// May be used to resume operation in future versions.
	// For now, status file is used to save valves status for next startup.
	// ────────────────────────────────────────────────────────────────────────────────
	void decodeJSON(const JsonVariant & source) override {
		const JsonArrayConst & source_valves = source[JsonKeys::VALVES].as<JsonArrayConst>();
		valves.resize(source_valves.size());
		copyArray(source_valves, valves.data(), valves.size());
	}

#pragma endregion JSONDECODABLE
#pragma region JSONENCODABLE
	static const char * encoderName() {
		return "Status";
	}

	static constexpr size_t encoderSize() {
		return ProgramSettings::STATUS_JSON_BUFFER_SIZE;
	}

	bool encodeJSON(const JsonVariant & dest) const override {
		using namespace JsonKeys;

		JsonArray doc_valves = dest.createNestedArray(VALVES);
		copyArray(valves.data(), valves.size(), doc_valves);

		return dest[VALVES_COUNT].set(valves.size())
			   && dest[SENSOR_PRESSURE].set(pressure)
			   && dest[SENSOR_TEMP].set(temperature)
			   && dest[SENSOR_BARO].set(barometric)
			   && dest[SENSOR_VOLUME].set(waterVolume)
			   && dest[SENSOR_DEPTH].set(waterDepth)
			   && dest[SENSOR_FLOW].set(waterFlow)
			   && dest[STATE_CURRENT_NAME].set(currentStateName)
			   && dest[TASK_CURRENT_NAME].set(currentTaskName);
	}

#pragma endregion JSONENCODABLE
#pragma region PRINTABLE
	size_t printTo(Print & printer) const override {
		StaticJsonDocument<encoderSize()> doc;
		JsonVariant object = doc.to<JsonVariant>();
		encodeJSON(object);
		return serializeJsonPretty(object, printer);
	}
#pragma endregion PRINTABLE
};