#pragma once
#include <KPFoundation.hpp>
#include <Application/Config.hpp>
#include <array>

#include <Utilities/JsonFileLoader.hpp>
#include <Valve/ValveStatus.hpp>
#include <Valve/ValveManager.hpp>

//
// ─── NOTE THIS OBJECT IS INTENDED TO BE READ-ONLY THAT PROVIDES SYSTEM-WIDE STATUS
//
class Status : public JsonDecodable,
			   public JsonEncodable,
			   public Printable,
			   public KPStateMachineListener,
			   public ValveManagerEventListner {
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

	void init(Config & config) {
		valves.resize(config.numberOfValves);
		memcpy(valves.data(), config.valves, sizeof(int) * config.numberOfValves);
	}

	//
	// ─── SECTION JSONDECODABLE COMPLIANCE ───────────────────────────────────────────
	//
	static const char * decoderName() {
		return "Status";
	}

	static constexpr size_t decoderSize() {
		return ProgramSettings::STATUS_JSON_BUFFER_SIZE;
	}

	// May be used to resume operation in future versions.
	// For now, status file is used to save valves status for next start up.
	void decodeJSON(const JsonVariant & source) override {
		const JsonArrayConst & source_valves = source[JsonKeys::VALVES].as<JsonArrayConst>();
		copyArray(source_valves, valves.data(), valves.size());
		valves.resize(source_valves.size());
	}

	// Update the content of status file
	void save(const char * filepath) const override {
		JsonFileLoader loader;
		loader.save(filepath, *this);
	}

	//
	// ─── SECTION JSONENCODABLE COMPLIANCE ───────────────────────────────────────────
	//
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
		// copyArray(_valves.data(), valves.size(), doc_valves);

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

	void load(const char * filepath) override {
		JsonFileLoader loader;
		loader.load(filepath, *this);
	}

	//
	// ─── SECTION PRINTABLE COMPLIANCE ───────────────────────────────────────────────
	//
	size_t printTo(Print & printer) const override {
		StaticJsonDocument<encoderSize()> doc;
		JsonVariant object = doc.to<JsonVariant>();
		encodeJSON(object);
		return serializeJsonPretty(object, printer);
	}

	//
	// ─── SECTION VALVEMANAGEREVENTLISTENER COMPLIANCE ───────────────────────────────
	//
private:
	void valvesChanged(const ValveManager & vm) override {
		currentValve = -1;
		for (const Valve & v : vm.valves) {
			if (v.status == ValveStatus::operating) {
				currentValve = v.id;
			}

			valves[v.id] = v.status;
		}
	}

	void stateTransitioned(const KPStateMachine & sm) override {
		currentStateName = sm.getCurrentState()->getName();
	}

public:
	// Override_Mode_Pin is connected to an external switch which is active low
	static bool isProgrammingMode() {
		return analogRead(Override_Mode_Pin) <= 100;
	}

};