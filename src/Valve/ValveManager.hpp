#pragma once
#include <KPFoundation.hpp>

#include <Application/Config.hpp>
#include <Application/Subject.hpp>
#include <Valve/Valve.hpp>
#include <Valve/ValveStatus.hpp>
#include <Valve/ValveObserver.hpp>
#include <Utilities/FileLoader.hpp>

#include <vector>

//
// ────────────────────────────────────────────────────────────────── I ──────────
//   :::::: V A L V E   M A N A G E R : :  :   :    :     :        :          :
//   ────────────────────────────────────────────────────────────────────────────
//

/** ────────────────────────────────────────────────────────────────────────────────
 * @brief This object providse APIs for keeping track of the valve system
 * 
**/
class ValveManager : public JsonEncodable,
					 public Subject<ValveObserver> {
public:
	std::vector<Valve> valves;
	const char * valveFolder = nullptr;

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Initialize ValveManager with the config object. This method sets
	 * status for each valve according to config object.
	 *
	 * @param config Config object containing meta information about the system
	**/
	void init(Config & config) {
		valveFolder = config.valveFolder;
		valves.resize(config.numberOfValves);
		for (size_t i = 0; i < valves.size(); i++) {
			valves[i].id = i;
			valves[i].setStatus(ValveStatus::Code(config.valves[i]));
		}
	}

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Set the status of the valve to the specified status 
	 *
	 * @param id Id of the valve
	 * @param status Status to set valve to
	**/
	void setValveStatus(int id, ValveStatus status) {
		valves[id].setStatus(status);

		updateObservers(&ValveObserver::valveDidUpdate, valves[id]);
	}

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Set the status of the valve to "free" if the valve is not yet sampled
	 *
	 * @param id Id of the valve (usally the index number)
	**/
	void setValveFreeIfNotYetSampled(int id) {
		if (valves[id].status != ValveStatus::sampled) {
			setValveStatus(id, ValveStatus::free);
			updateObservers(&ValveObserver::valveDidUpdate, valves[id]);
		}
	}

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Update valve objects from JSON array
	 *
	 * @param task_array JSON Array encoded by ArduinoJson 
	**/
	void updateValves(const JsonArray & task_array) {
		for (const JsonObject & object : task_array) {
			int id = object[JsonKeys::VALVE_ID];
			if (valves[id].status != ValveStatus::sampled) {
				valves[id].decodeJSON(object);
			} else {
				PrintConfig::setPrintVerbose(Verbosity::info);
				println("Valve is already sampled");
				PrintConfig::setDefaultVerbose();
			}
		}

		updateObservers(&ValveObserver::valveArrayDidUpdate, valves);
	}

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Reading and decode each JSON file in the given directory to
	 * corresponding valve object.
	 *
	 * @param _dir Path to the valve folder (default=~/valves)
	**/
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

		PrintConfig::setPrintVerbose(Verbosity::debug);
		println("\033[1;32mValveManager\033[0m: finished reading in ", millis() - start, " ms");
		PrintConfig::setDefaultVerbose();

		updateObservers(&ValveObserver::valveArrayDidUpdate, valves);
	}

	/** ────────────────────────────────────────────────────────────────────────────────
	 * @brief Encode and store each valve object to corresponding JSON file in the
	 * given directory
	 *
	 * @param _dir Path to the valve folder (default=~/valves)
	**/
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

		PrintConfig::setPrintVerbose(Verbosity::debug);
		println("\033[1;32mValveManager\033[0m: finished writing in ", millis() - start, " ms");
		PrintConfig::setDefaultVerbose();

		updateObservers(&ValveObserver::valveArrayDidUpdate, valves);
	}

#pragma region JSONENCODABLE
	static const char * encoderName() {
		return "ValveManager";
	}

	static constexpr size_t encoderSize() {
		return Valve::encoderSize() * ProgramSettings::MAX_VALVES;
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