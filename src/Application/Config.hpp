#pragma once

#include <KPFoundation.hpp>
#include <KPDataStoreInterface.hpp>
#include <Application/Constants.hpp>
#include <Utilities/JsonFileLoader.hpp>

#include <ArduinoJson.h>

// ────────────────────────────────────────────────────────────────────────────────
// ─── SECTION CONFIG ─────────────────────────────────────────────────────────────
// ────────────────────────────────────────────────────────────────────────────────
//
// NOTE This object is intended to be read-only and represents to actual data in the
// config file in SD card
// ────────────────────────────────────────────────────────────────────────────────
class Config : public JsonDecodable, public JsonEncodable, public Printable {
public:
	int valveUpperBound;
	int numberOfValves;
	int valves[ProgramSettings::MAX_VALVES]{0};

	const char * configFilepath;

	char logFile[ProgramSettings::SD_FILE_NAME_LENGTH]{0};
	char statusFile[ProgramSettings::SD_FILE_NAME_LENGTH]{0};
	char taskFolder[ProgramSettings::SD_FILE_NAME_LENGTH]{0};
	char valveFolder[ProgramSettings::SD_FILE_NAME_LENGTH]{0};

	bool shutdownOverride = true;

public:
	Config()			   = delete;
	Config(const Config &) = delete;
	Config & operator=(const Config &) = delete;

	explicit Config(const char * configFilepath)
		: configFilepath(configFilepath) {
	}

	static const char * decoderName() {
		return "Config";
	}

	static constexpr size_t decoderSize() {
		return ProgramSettings::CONFIG_JSON_BUFFER_SIZE;
	}

	void decodeJSON(const JsonVariant & source) override {
		using namespace ProgramSettings;
		using namespace ConfigKeys;

		valveUpperBound = source[VALVE_UPPER_BOUND];
		numberOfValves	= valveUpperBound + 1;

		// <- valves
		JsonArrayConst config_valves = source[VALVES_FREE].as<JsonArrayConst>();
		for (int freeValveId : config_valves) {
			if (freeValveId < 0) {
				KPStringBuilder<120> message("Config: ", freeValveId, " < 0 ");
				raise(Error(message));
			}

			if (freeValveId > valveUpperBound) {
				KPStringBuilder<120> message("Config: ", freeValveId, " > ", valveUpperBound);
				raise(Error(message));
			} else {
				valves[freeValveId] = 1;
			}
		}

		strncpy(logFile, source[FILE_LOG], SD_FILE_NAME_LENGTH);
		strncpy(statusFile, source[FILE_STATUS], SD_FILE_NAME_LENGTH);
		strncpy(taskFolder, source[FOLDER_TASK], SD_FILE_NAME_LENGTH);
		strncpy(valveFolder, source[FOLDER_VALVE], SD_FILE_NAME_LENGTH);
	}

	void load(const char * filepath = nullptr) override {
		JsonFileLoader loader;
		loader.load(configFilepath, *this);
	}

	static const char * encoderName() {
		return "config";
	}

	static constexpr size_t encoderSize() {
		return ProgramSettings::CONFIG_JSON_BUFFER_SIZE;
	}

	bool encodeJSON(const JsonVariant & dest) const override {
		using namespace ConfigKeys;

		// -> valves
		JsonArray array_array = dest.createNestedArray(VALVES_FREE);
		copyArray(valves, array_array);

		return dest[VALVE_UPPER_BOUND].set(valveUpperBound)
			   && dest[FILE_LOG].set((char *) logFile)
			   && dest[FILE_STATUS].set((char *) statusFile)
			   && dest[FOLDER_TASK].set((char *) taskFolder)
			   && dest[FOLDER_VALVE].set((char *) valveFolder);
	}

	size_t printTo(Print & p) const override {
		using namespace ConfigKeys;
		StaticJsonDocument<encoderSize()> doc;
		encodeJSON(doc.to<JsonVariant>());

		return serializeJsonPretty(doc, Serial);
	}
};