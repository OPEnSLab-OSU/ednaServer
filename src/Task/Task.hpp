#pragma once
#include <KPArray.hpp>
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

	char name[TaskSettings::NAME_LENGTH]{0};
	char notes[TaskSettings::NOTES_LENGTH]{0};

	long creation = 0;
	long schedule = 0;

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
	int m_currentValvePosition = 0;

public:
	Task()					 = default;
	Task(const Task & other) = default;
	Task & operator=(const Task &) = default;

	explicit Task(const JsonObject & data) {
		decodeJSON(data);
	}

	void markAsCompleted() {
		valves.clear();
		status = TaskStatus::completed;
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Set next schedule and move on to the next valve. If no more valve, set the task
	// as completed
	// ────────────────────────────────────────────────────────────────────────────────
	void next() {
		schedule = now() + std::max(timeBetween, 5);
		if (++m_currentValvePosition >= size()) {
			markAsCompleted();
		}
	}

	int currentValvePosition() const {
		return m_currentValvePosition;
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Returns the number of valves assigned to this task
	// ────────────────────────────────────────────────────────────────────────────────
	int size() const {
		return valves.size();
	}

	bool isCompleted() const {
		return status == TaskStatus::completed;
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Returns -1 if no more valve, otherwise returns the valve number
	// ────────────────────────────────────────────────────────────────────────────────
	int getCurrentValveIndex() const {
		return (currentValvePosition() >= size()) ? -1 : valves[currentValvePosition()];
	}

#pragma region JSONDECODABLE
	static const char * decoderName() {
		return "Task";
	}

	static constexpr size_t decoderSize() {
		return ProgramSettings::TASK_JSON_BUFFER_SIZE;
	}

	void load(const char * filepath) override {
		JsonFileLoader loader;
		loader.load(filepath, *this);
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
			m_currentValvePosition = source["currentValvePosition"];
		}

		creation	   = source["creation"];
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

	void save(const char * filepath) const override {
		JsonFileLoader loader;
		loader.save(filepath, *this);
	}

	bool encodeJSON(const JsonVariant & dest) const override {
		return dest["name"].set((char *) name)
			   && dest["notes"].set((char *) notes)
			   && dest["status"].set(status)
			   && dest["creation"].set(creation)
			   && dest["schedule"].set(schedule)
			   && dest["flushTime"].set(flushTime)
			   && dest["flushVolume"].set(flushVolume)
			   && dest["sampleTime"].set(sampleTime)
			   && dest["samplePressure"].set(samplePressure)
			   && dest["sampleVolume"].set(sampleVolume)
			   && dest["dryTime"].set(dryTime)
			   && dest["preserveTime"].set(preserveTime)
			   && dest["timeBetween"].set(timeBetween)
			   && dest["currentValvePosition"].set(currentValvePosition())
			   && copyArray(valves.data(), valves.size(), dest.createNestedArray("valves"));
	}

	size_t printTo(Print & printer) const override {
		StaticJsonDocument<encoderSize()> doc;
		JsonVariant object = doc.to<JsonVariant>();
		encodeJSON(object);
		return serializeJsonPretty(doc, Serial);
	}
#pragma endregion
};