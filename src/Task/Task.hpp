#pragma once
#include <KPArray.hpp>
#include <KPFoundation.hpp>
#include <array>

#include <ArduinoJson.h>

#include <Application/Constants.hpp>
#include <Utilities/JsonEncodableDecodable.hpp>
#include <Utilities/JsonFileLoader.hpp>

#include <Task/TaskStatus.hpp>

struct Taskref;
struct Task : public JsonEncodable, public JsonDecodable, public Printable {
	char name[TaskSettings::NAME_LENGTH]{0};
	char notes[TaskSettings::NOTES_LENGTH]{0};

	std::array<uint8_t, 24> valves;
	int8_t valveCount = 0;

	bool deleteOnCompletion = false;

public:
	int8_t currentValveIndex = -1;

public:
	int status		 = TaskStatus::inactive;  // 0=inactive, 1=active, 2=completed
	long creation	 = 0;
	long schedule	 = 0;
	long timeBetween = 0;

	unsigned long flushTime		 = 0;
	unsigned long flushVolume	 = 0;
	unsigned long sampleTime	 = 0;
	unsigned long samplePressure = 0;
	unsigned long sampleVolume	 = 0;
	unsigned long dryTime		 = 0;
	unsigned long preserveTime	 = 0;

	Task()					 = default;
	Task(const Task & other) = default;
	Task & operator=(const Task &) = default;

	explicit Task(const JsonObject & data) {
		decodeJSON(data);
	}

	void markAsCompleted() {
		status			  = TaskStatus::completed;
		valveCount		  = 0;
		currentValveIndex = -1;
	}

	void nextValve() {
		schedule = now() + std::max(timeBetween, 5l);
		if (++currentValveIndex >= valveCount) {
			markAsCompleted();
		}
	}

	int getValveIndex() {
		if (currentValveIndex == -1 || currentValveIndex >= valveCount) {
			return -1;
		}

		return valves[currentValveIndex];
	}

	//
	// ─── SECTION JSONDECODABLE COMPLIANCE ───────────────────────────────────────────
	//

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
			copyArray(valve_array, valves.data(), valve_array.size());
			valveCount		  = valve_array.size();
			currentValveIndex = source["currentValveIndex"];
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

	//
	// ─── SECTION JSONENCODABLE COMPLIANCE ───────────────────────────────────────────
	//

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
			   && copyArray(valves.data(), valveCount, dest.createNestedArray("valves"))
			   && dest["currentValveIndex"].set(currentValveIndex);
	}

	size_t printTo(Print & printer) const override {
		StaticJsonDocument<encoderSize()> doc;
		JsonVariant object = doc.to<JsonVariant>();
		encodeJSON(object);
		return serializeJsonPretty(doc, Serial);
	}
};