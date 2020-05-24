#pragma once
#include <KPFoundation.hpp>

#include <Application/Config.hpp>
#include <Valve/Valve.hpp>
#include <Valve/ValveStatus.hpp>
#include <Utilities/FileLoader.hpp>
#include <vector>

// ────────────────────────────────────────────────────────────────────────────────
// This is an extremely simple event listener. This will be updated to a pub-sub
// model later.
// ────────────────────────────────────────────────────────────────────────────────
class ValveManager;
class ValveManagerEventListner {
public:
	virtual void valvesChanged(const ValveManager & vm) = 0;
};

//
// ────────────────────────────────────────────────────────────────── I ──────────
//   :::::: V A L V E   M A N A G E R : :  :   :    :     :        :          :
// ────────────────────────────────────────────────────────────────────────────
//
// This object providse APIs for keeping track of the valve system
//
class ValveManager : public JsonEncodable {
private:
	std::vector<ValveManagerEventListner *> listeners;

public:
	// TODO: Change to vector due to its flexibility
	constexpr static size_t MAX_VALVES = ProgramSettings::MAX_VALVES;
	std::array<Valve, MAX_VALVES> valves;
	const char * valveFolder = nullptr;

	void init(Config & config) {
		valveFolder = config.valveFolder;
		for (size_t i = 0; i < valves.size(); i++) {
			valves[i].id = i;
			valves[i].setStatus(ValveStatus::Code(config.valves[i]));
		}
	}

	void updateListeners() {
		for (auto listener : listeners) {
			listener->valvesChanged(*this);
		}
	}

	void addListener(ValveManagerEventListner * listener) {
		listeners.push_back(listener);
	}

	void setValveStatus(int id, ValveStatus status) {
		valves[id].setStatus(status);
		updateListeners();
	}

	void setValveFreeIfNotYetSampled(int id) {
		if (valves[id].status != ValveStatus::sampled) {
			setValveStatus(id, ValveStatus::free);
		}
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Update valve objects from JSON array
	// ────────────────────────────────────────────────────────────────────────────────
	void updateValves(const JsonArray & task_array) {
		for (const JsonObject & object : task_array) {
			int id = object[JsonKeys::VALVE_ID];
			if (valves[id].status != ValveStatus::sampled) {
				valves[id].decodeJSON(object);
			} else {
				PrintConfig::printVerbose = Verbosity::debug;
				println("Valve is already sampled");
				PrintConfig::printVerbose = PrintConfig::defaultPrintVerbose;
			}
		}

		updateListeners();
	}

	// TODO: Would index file be useful here?
	void loadIndexFile(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : valveFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Reading and decode each JSON file in the given directory to corresponding valve
	// object.
	// ────────────────────────────────────────────────────────────────────────────────
	void loadValvesFromDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : valveFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		unsigned long start = millis();
		for (size_t i = 0; i < valves.size(); i++) {
			KPStringBuilder<32> filename("valve-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			valves[i].load(filepath);
		}

		println("\033[1;32mValveManager\033[0m: finished reading in ", millis() - start, " ms");
		updateListeners();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Encode and store each valve object to corresponding JSON file in the given directory
	// @param _dir
	// ────────────────────────────────────────────────────────────────────────────────
	void writeValvesToDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : valveFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		unsigned long start = millis();
		for (size_t i = 0; i < valves.size(); i++) {
			KPStringBuilder<32> filename("valve-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			valves[i].save(filepath);
		}

		println("\033[1;32mValveManager\033[0m: finished writing in ", millis() - start, " ms");
		updateListeners();
	}

#pragma region JSONENCODABLE
	static const char * encoderName() {
		return "ValveManager";
	}

	static constexpr size_t encoderSize() {
		return Valve::encoderSize() * MAX_VALVES;
	}

	bool encodeJSON(const JsonVariant & dest) const {
		for (const auto & v : valves) {
			if (!v.encodeJSON(dest.createNestedObject())) {
				return false;
			}
		}

		return true;
	}
#pragma endregion
};