#pragma once
#include <KPArray.hpp>
#include <KPFoundation.hpp>

#include <ArduinoJson.h>

#include <Application/Constants.hpp>
#include <Utilities/JsonEncodableDecodable.hpp>

#include <Valve/ValveStatus.hpp>

//
// ─── SECTION VALVE OBJECT ───────────────────────────────────────────────────────
//
struct Valve : public JsonEncodable, public JsonDecodable, public Printable {
public:
	int id	   = ValveStatus::unavailable;
	int status = ValveStatus::unavailable;
	char group[ProgramSettings::VALVE_GROUP_LENGTH]{0};

	Valve()					   = default;
	Valve(const Valve & other) = default;
	Valve & operator=(const Valve &) = default;

	explicit Valve(const JsonObject & data) {
		decodeJSON(data);
	}

	void setStatus(ValveStatus vs) {
		status = vs;
	}

	//
	// ─── SECTION JSONDECODABLE COMPLIANCE ───────────────────────────────────────────
	//

	static const char * decoderName() {
		return "Valve";
	}

	static constexpr size_t decoderSize() {
		return ProgramSettings::VALVE_JSON_BUFFER_SIZE;
	}

	void load(const char * filepath) override {
		JsonFileLoader loader;
		loader.load(filepath, *this);
	}

	void decodeJSON(const JsonVariant & src) override {
		using namespace JsonKeys;
		// -> group
		strncpy(group, src[VALVE_GROUP], ProgramSettings::VALVE_GROUP_LENGTH);
		if (group[ProgramSettings::VALVE_GROUP_LENGTH - 1] != 0) {
			println("Warning (Valve): Group name exceeds its buffer size and will be truncated");
		}

		status = src[VALVE_STATUS];
	}

	//
	// ─── SECTION JSONENCODABLE COMPLIANCE ───────────────────────────────────────────
	//

	const char * encoderName() const {
		return "Valve";
	}

	static constexpr size_t encoderSize() {
		return ProgramSettings::VALVE_JSON_BUFFER_SIZE;
	}

	bool encodeJSON(const JsonVariant & dst) const override {
		using namespace JsonKeys;
		return dst[VALVE_ID].set(id)
			   && dst[VALVE_GROUP].set((char *) group)
			   && dst[VALVE_STATUS].set(status);
	}

	void save(const char * filepath) const override {
		JsonFileLoader loader;
		loader.save(filepath, *this);
	}

	size_t printTo(Print & printer) const override {
		StaticJsonDocument<encoderSize()> doc;
		JsonVariant object = doc.to<JsonVariant>();
		encodeJSON(object);
		return serializeJsonPretty(object, Serial);
	}
};