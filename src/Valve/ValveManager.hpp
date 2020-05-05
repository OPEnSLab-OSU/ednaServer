#pragma once
#include <KPFoundation.hpp>
#include <KPDataStoreInterface.hpp>
#include <KPArray.hpp>

#include <Application/Config.hpp>
#include <Valve/Valve.hpp>
#include <Valve/ValveStatus.hpp>

#include <Utilities/FileLoader.hpp>

#include <vector>

class ValveManager;
class ValveManagerEventListner {
public:
	virtual void valvesChanged(const ValveManager & vm) = 0;
};

//
// ─── SECTION VALVE MANAGER ──────────────────────────────────────────────────────
//
// NOTE: This object provide APIs for keeping track of the valve system
// ────────────────────────────────────────────────────────────────────────────────
class ValveManager : public JsonEncodable {
private:
	std::vector<ValveManagerEventListner *> listeners;

public:
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

	void setValveSampled(int id) {
		setValveStatus(id, ValveStatus::sampled);
	}

	void setValveOperating(int id) {
		setValveStatus(id, ValveStatus::operating);
	}

	void setValveStatus(int id, ValveStatus status) {
		valves[id].setStatus(status);
		updateListeners();
	}

	void freeIfNotYetSampled(int id) {
		if (valves[id].status != ValveStatus::sampled) {
			valves[id].setStatus(ValveStatus::free);
		}
	}

	std::vector<int> filter(std::function<bool(const Valve &)> pred) {
		std::vector<int> list;
		for (size_t i = 0; i < valves.size(); i++) {
			if (pred(valves[i])) {
				list.push_back(i);
			}
		}
		return list;
	}

	void loadValvesFromDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : valveFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		unsigned long start = millis();
		for (size_t i = 0; i < valves.size(); i++) {
			KPStringBuilder<32> filename("valve-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			valves[i].load(filepath);
			valves[i].id = i;
		}

		println("\033[1;32mValveManager\033[0m: finished loading in ", millis() - start, " ms");
		updateListeners();
	}

	void updateValves(const JsonArray & task_array) {
		for (const JsonObject & object : task_array) {
			int id = object[JsonKeys::VALVE_ID];
			if (valves[id].status) {
				valves[id].decodeJSON(object);
			} else {
				println("Valve is already sampled");
			}
		}

		updateListeners();
	}

	void saveValvesToDirectory(const char * dir) {
		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		unsigned long start = millis();
		for (size_t i = 0; i < valves.size(); i++) {
			KPStringBuilder<32> filename("valve-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			valves[i].save(filepath);
		}

		println("ValveManager: finished saving in ", millis() - start, " ms");
		updateListeners();
	}

	//
	// ─── JSONENCODABLE ──────────────────────────────────────────────────────────────
	//

	static const char * encoderName() {
		return "ValveManager";
	}

	static constexpr size_t encoderSize() {
		return Valve::encoderSize() * MAX_VALVES;
	}

	bool encodeJSON(const JsonVariant & dst) const {
		for (const auto & v : valves) {
			if (!v.encodeJSON(dst.createNestedObject())) {
				return false;
			}
		}

		return true;
	}
};