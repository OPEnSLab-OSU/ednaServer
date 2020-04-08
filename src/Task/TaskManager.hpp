#pragma once
#include <KPFoundation.hpp>
#include <KPDataStoreInterface.hpp>
#include <KPArray.hpp>

#include <Task/Task.hpp>
#include <vector>

#include <Application/Config.hpp>

#include "SD.h"

class TaskVerificationError {
public:
	byte bitmask;

	// Allow for implicit conversion from ValveStatus::code -> ValveStatus(ValveStatus::code)
	// Meaning that the client code can call function that accept ValveStatus with just ValveStatus::[enum]
	void setTime() {
		bitmask |= 1 << 1;
	}
};

class TaskManager : public KPComponent, public JsonEncodable {
public:
	static constexpr int numberOfTasks = 10;
	KPArray<Task, numberOfTasks> tasks;

	char * taskFolder = nullptr;

	TaskManager()
		: KPComponent("TaskManager") {
		tasks.resize(numberOfTasks);
	}

	void init(Config & config) {
		taskFolder = config.taskFolder;
	}

	int findTaskWithName(const char * name) {
		for (int i = 0; i < tasks.size(); i++) {
			if (strcmp(tasks[i].name, name) == 0) {
				return i;
			}
		}

		return -1;
	}

	void loadTasksFromDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		// Load task index file
		KPStringBuilder<32> indexFilepath(dir, "/index.js");
		File indexFile = SD.open(indexFilepath);

		// Read index file
		StaticJsonDocument<500> doc;
		deserializeJson(doc, indexFile);
		indexFile.close();

		// load the first "count" tasks
		int count = doc["count"];
		for (int i = 0; i < count; i++) {
			KPStringBuilder<32> filename("task-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			tasks[i].load(filepath);
		}

		// Resize tasks array
		tasks.resize(count);
	}

	void updateTasks(const JsonArrayConst & task_array) {
		for (const JsonObjectConst & object : task_array) {
			int id = object["valve"];
			if (tasks[id].status) {
				tasks[id].decodeJSON(object);
			} else {
				println("Task is already inactive");
			}
		}
	}

	// Performe identity check and update the task
	int updateTaskWithData(JsonDocument & data, JsonDocument & response) {
		using namespace JsonKeys;
		int index = findTaskWithName(data[TASK_NAME]);
		if (index == -1) {
			response["error"] = "No task with such name";
			return -1;
		}

		if (data.containsKey(TASK_NEW_NAME)) {
			// check there is a task with newName then
			// replaces the name with new name if none is found
			if (findTaskWithName(data[TASK_NEW_NAME]) == -1) {
				data[TASK_NAME] = data[TASK_NEW_NAME];
				goto save;
			}

			KPStringBuilder<100> error("Task with name ", data[TASK_NEW_NAME].as<char *>(), " already exist");
			response["error"] = (char *)error.c_str();
			return -1;
		}

	save:
		JsonVariant payload = response.createNestedObject("payload");
		tasks[index]		= Task(data.as<JsonObjectConst>());
		tasks[index].encodeJSON(payload);

		KPStringBuilder<100> success("Saved", data[TASK_NAME].as<char *>());
		response["success"] = (char *)success.c_str();
		return index;
	}

	// NOTE: Validate
	int validateTask(Task & task) {
		// Application & app = *static_cast<Application *>(controller);
		// task.schedule > now
		if (task.schedule < now() && task.schedule != -1) {
		}
	}

	void createTask(const JsonObjectConst & object) {
		Task task(object);
		tasks.append(task);
		writeTaskArrayToDirectory();
	}

	void updateIndexFile(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		// Update the index file
		KPStringBuilder<32> indexFilepath(dir, "/index.js");
		File indexFile = SD.open(indexFilepath, FILE_WRITE);
		StaticJsonDocument<100> index_doc;
		index_doc["count"] = tasks.size();
		serializeJson(index_doc, indexFile);
		indexFile.close();
	}

	void writeTaskArrayToDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		FileLoader loader;
		loader.createDirectoryIfNeeded(dir);
		for (int i = 0; i < tasks.size(); i++) {
			KPStringBuilder<32> filename("task-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			tasks[i].save(filepath);
		}

		updateIndexFile(dir);
	}

	//
	// ─── SECTION JSONDECODABLE COMPLIANCE ──────────────────────────────────────────────
	//

	const char * encoderName() const {
		return "TaskManager";
	}

	bool encodeJSON(JsonVariant & dest) const override {
		for (const Task & t : tasks) {
			JsonVariant obj = dest.createNestedObject();
			t.encodeJSON(obj);
		}

		return true;
	}
};

// virtual const char * encoderName() const {
// 		return "Unnamed";
// 	}

// 	virtual bool encodeJSON(JsonObject & dest) const = 0;
// 	virtual void save(const char * filepath) const {
// 		raise(Error("JsonEncodable save needs override"));
// 	}