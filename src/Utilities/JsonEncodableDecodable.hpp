#pragma once

#include <KPFoundation.hpp>
#include <ArduinoJson.h>

// ────────────────────────────────────────────────────────────────────────────────
// ─── SECTION  INTERFACE FOR CUSTOM JSON DECODING OBJECT ─────────────────────────
// ────────────────────────────────────────────────────────────────────────────────
class JsonDecodable {
public:
	static const char * decoderName() {
		return "Unnamed";
	}

	static constexpr size_t decoderSize();
	virtual void decodeJSON(const JsonVariant & source) = 0;
	virtual void load(const char * filepath) {
		raise(Error("JsonDecodable load needs override"));
	}
};

inline void decodeJSON(JsonDecodable & decoder, const JsonVariant & source) {
	decoder.decodeJSON(source);
};

// ────────────────────────────────────────────────────────────────────────────────
// ─── SECTION  INTERFACE FOR CUSTOM JSON ENCODING OBJECT ─────────────────────────
// ────────────────────────────────────────────────────────────────────────────────
class JsonEncodable {
public:
	static const char * encoderName() {
		return "Unnamed";
	}

	static const size_t encoderSize();
	virtual bool encodeJSON(const JsonVariant & dest) const = 0;
	virtual void save(const char * filepath) const {
		raise(Error("JsonEncodable save needs override"));
	}
};

inline void encodeJSON(JsonEncodable & encoder, const JsonVariant & dest) {
	encoder.encodeJSON(dest);
};
