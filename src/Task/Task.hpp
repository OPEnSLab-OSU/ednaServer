#pragma once
#include <KPFoundation.hpp>
#include <array>

#include <ArduinoJson.h>

#include <Application/Constants.hpp>
#include <Utilities/JsonEncodableDecodable.hpp>
#include <Utilities/JsonFileLoader.hpp>

#include <Task/TaskStatus.hpp>

struct Task : public JsonEncodable, public JsonDecodable, public Printable {
public:
	bool deleteOnCompletion = false;

	int id;
	char name[TaskSettings::NAME_LENGTH]{0};
	char notes[TaskSettings::NOTES_LENGTH]{0};

	long createdAt = 0;
	long schedule  = 0;

	int status		= TaskStatus::inactive;
	int timeBetween = 0;

	int flushTime	   = 0;
	int flushVolume	   = 0;
	int sampleTime	   = 0;
	int sampleVolume   = 0;
	int samplePressure = 0;
	int dryTime		   = 0;
	int preserveTime   = 0;

	std::vector<uint8_t> valves;

private:
	int m_valveOffset = 0;

public:
	Task()					 = default;
	Task(const Task & other) = default;
	Task & operator=(const Task &) = default;

	explicit Task(const JsonObject & data) {
		decodeJSON(data);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set next schedule and move on to the next valve. If no more valve, set the task
	// as completed
	// ────────────────────────────────────────────────────────────────────────────────
	// void next() {
	// 	schedule = now() + std::max(timeBetween, 5);
	// 	if (++m_valveOffset >= numberOfValves()) {
	// 		markAsCompleted();
	// 	}
	// }

	int valveOffset() const {
		return m_valveOffset;
	}

	int numberOfValves() const {
		return valves.size();
	}

	bool isCompleted() const {
		return status == TaskStatus::completed;
	}

	/** ────────────────────────────────────────────────────────────────────────────
	 *  @brief Get the Current Valve ID
	 *  
	 *  @return int -1 if no more valve, otherwise returns the valve number
	 *  ──────────────────────────────────────────────────────────────────────────── */
	int getCurrentValveId() const {
		return (valveOffset() >= numberOfValves()) ? -1 : valves[valveOffset()];
	}

#pragma region JSONDECODABLE
	static const char * decoderName() {
		return "Task";
	}

	static constexpr size_t decoderSize() {
		return ProgramSettings::TASK_JSON_BUFFER_SIZE;
	}

	void decodeJSON(const JsonVariant & source) override {
		using namespace TaskSettings;

		if (source.containsKey("name")) {
			snprintf(name, NAME_LENGTH, "%s", source["name"].as<char *>());
		}

		if (source.containsKey("notes")) {
			snprintf(notes, NOTES_LENGTH, "%s", source["notes"].as<char *>());
		}

		if (source.containsKey("valves")) {
			JsonArray valve_array = source["valves"].as<JsonArray>();
			valves.resize(valve_array.size());
			copyArray(valve_array, valves.data(), valve_array.size());
			m_valveOffset = source["valveOffset"];
		}

		id			   = source["id"];
		createdAt	   = source["createdAt"];
		schedule	   = source["schedule"];
		status		   = source["status"];
		flushTime	   = source["flushTime"];
		flushVolume	   = source["flushVolume"];
		sampleTime	   = source["sampleTime"];
		samplePressure = source["samplePressure"];
		sampleVolume   = source["sampleVolume"];
		dryTime		   = source["dryTime"];
		preserveTime   = source["preserveTime"];
		timeBetween	   = source["timeBetween"];
	}
#pragma endregion
#pragma region JSONENCODABLE

	static const char * encoderName() {
		return "Task";
	}

	static constexpr size_t encoderSize() {
		return ProgramSettings::TASK_JSON_BUFFER_SIZE;
	}

	bool encodeJSON(const JsonVariant & dst) const override {
		return dst["id"].set(id)
			   && dst["name"].set((char *) name)
			   && dst["notes"].set((char *) notes)
			   && dst["status"].set(status)
			   && dst["creation"].set(createdAt)
			   && dst["schedule"].set(schedule)
			   && dst["flushTime"].set(flushTime)
			   && dst["flushVolume"].set(flushVolume)
			   && dst["sampleTime"].set(sampleTime)
			   && dst["samplePressure"].set(samplePressure)
			   && dst["sampleVolume"].set(sampleVolume)
			   && dst["dryTime"].set(dryTime)
			   && dst["preserveTime"].set(preserveTime)
			   && dst["timeBetween"].set(timeBetween)
			   && dst["valveOffset"].set(valveOffset())
			   && copyArray(valves.data(), valves.size(), dst.createNestedArray("valves"));
	}

	size_t printTo(Print & printer) const override {
		StaticJsonDocument<encoderSize()> doc;
		JsonVariant object = doc.to<JsonVariant>();
		encodeJSON(object);
		return serializeJsonPretty(doc, Serial);
	}
#pragma endregion
};