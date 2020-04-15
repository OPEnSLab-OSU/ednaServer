#pragma once
#include <KPFoundation.hpp>
#include <KPDataStoreInterface.hpp>
#include <KPArray.hpp>

#include <Task/Task.hpp>
#include <vector>

#include <Application/Config.hpp>

#include "SD.h"

class TaskError {
public:
	char message[256];
	bool error	 = false;
	bool success = false;

	TaskError(const char * message, bool isError) {
		strncpy(this->message, message, 255);
		this->message[255] = 0;
		this->error		   = isError;
		this->success	   = !isError;
	}

	static TaskError CreateError(const char * message) {
		return TaskError("ok", true);
	}
};

class TaskManager : public KPComponent,
					public JsonEncodable,
					public Printable {
public:
	std::vector<Task> tasks;
	const char * taskFolder = nullptr;

	TaskManager()
		: KPComponent("TaskManager") {}

	void init(Config & config) {
		taskFolder = config.taskFolder;
	}

	int findTaskWithName(const char * name) {
		auto iter = std::find_if(tasks.begin(), tasks.end(), [name](const Task & t) {
			return strcmp(t.name, name) == 0;
		});

		if (iter != tasks.end()) {
			return std::distance(tasks.begin(), iter);
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
		tasks.resize(count);
		for (int i = 0; i < count; i++) {
			KPStringBuilder<32> filepath(dir, "/", "task-", i, ".js");
			tasks[i].load(filepath);
		}
	}

	void updateTasks(const JsonArray & task_array) {
		for (const JsonObject & object : task_array) {
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
			// Check there is a task with newName then
			// replaces the name with new name if none is found
			if (findTaskWithName(data[TASK_NEW_NAME]) == -1) {
				data[TASK_NAME] = data[TASK_NEW_NAME];
				goto save;
			}

			KPStringBuilder<64> error("Task with name ", data[TASK_NEW_NAME].as<char *>(), " already exist");
			response["error"] = (char *) error.c_str();
			return -1;
		}

	save:
		JsonVariant payload = response.createNestedObject("payload");
		tasks[index]		= Task(data.as<JsonObject>());
		tasks[index].encodeJSON(payload);

		KPStringBuilder<100> success("Saved", data[TASK_NAME].as<char *>());
		response["success"] = (char *) success.c_str();
		return index;
	}

	// NOTE: Validate Task

	int validateTask(Task & task) {
		// Application & app = *static_cast<Application *>(controller);
		// task.schedule > now
		if (task.schedule < now() && task.schedule != -1) {
		}
		return 0;
	}

	int validateTask(Task & task, JsonDocument & response) {
		return 0;
	}

	void unschedule(int index) {
	}

	void remove(int index) {
	}

	void next() {
	}

	void createTask(const JsonObject & src) {
		tasks.push_back(Task(src));
		writeTaskArrayToDirectory();
	}

	void updateIndexFile(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		JsonFileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		// Update the index file
		KPStringBuilder<32> indexFilepath(dir, "/index.js");
		StaticJsonDocument<100> indexJson;
		indexJson["count"] = tasks.size();
		loader.save(indexFilepath, indexJson);
	}

	void writeTaskArrayToDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		JsonFileLoader loader;
		loader.createDirectoryIfNeeded(dir);
		for (size_t i = 0; i < tasks.size(); i++) {
			KPStringBuilder<32> filename("task-", i, ".js");
			KPStringBuilder<64> filepath(dir, "/", filename);
			loader.save(filepath, tasks[i]);
		}

		updateIndexFile(dir);
	}

	void deleteTask(int index) {
		if (index < 0 || index >= tasks.size()) {
			raise("(TaskManager: delete) Index out of range");
		}

		tasks.erase(tasks.begin() + index);
	}

	//
	// ─── SECTION JSONDECODABLE COMPLIANCE ──────────────────────────────────────────────
	//

	static const char * encoderName() {
		return "TaskManager";
	}

	static constexpr size_t encoderSize() {
		return Task::encoderSize() * 10;
	}

	bool encodeJSON(const JsonVariant & dst) const override {
		for (const Task & t : tasks) {
			JsonVariant obj = dst.createNestedObject();
			if (!t.encodeJSON(obj)) {
				return false;
			}
		}

		return true;
	}

	size_t printTo(Print & p) const {
		size_t charWritten = 0;
		charWritten += p.println("[");
		for (auto & task : tasks) {
			charWritten += p.print(task);
			charWritten += p.println(",");
		}
		charWritten += p.println("]");
		return charWritten;
	}
};

// virtual const char * encoderName() const {
// 		return "Unnamed";
// 	}

// 	virtual bool encodeJSON(JsonObject & dest) const = 0;
// 	virtual void save(const char * filepath) const {
// 		raise(Error("JsonEncodable save needs override"));
// 	}