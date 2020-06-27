#pragma once
#include <KPFoundation.hpp>
#include <KPDataStoreInterface.hpp>

#include <Task/Task.hpp>
#include <Task/TaskObserver.hpp>
#include <Application/Config.hpp>

#include <vector>
#include "SD.h"

/**
 * @brief Manages and keeping track of tasks
 * 
 */
class TaskManager : public KPComponent,
					public JsonEncodable,
					public Printable,
					public Subject<TaskObserver> {
public:
	const char * taskFolder = nullptr;
	std::vector<Task> tasks;
	std::vector<TaskObserver *> listeners;

	TaskManager()
		: KPComponent("TaskManager") {}

	void init(Config & config) {
		taskFolder = config.taskFolder;
	}

	/**
	 * @brief Return index of the task with name
	 * 
	 * @param name name of the task
	 * @return int index of the task in the task array, otherwise -1.
	 */
	int findTaskWithName(const char * name) {
		auto iter = std::find_if(tasks.begin(), tasks.end(), [name](const Task & t) {
			return strcmp(t.name, name) == 0;
		});

		auto d = std::distance(tasks.begin(), iter);
		return (size_t) d == tasks.size() ? -1 : d;
	}

	void loadTasksFromDirectory(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		JsonFileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		// Load task index file and set the number of tasks
		KPStringBuilder<32> indexFilepath(dir, "/index.js");
		StaticJsonDocument<100> indexFile;
		loader.load(indexFilepath, indexFile);
		tasks.resize(indexFile["count"]);

		// Decode each task object into memory
		for (unsigned int i = 0; i < tasks.size(); i++) {
			KPStringBuilder<32> filepath(dir, "/task-", i, ".js");
			tasks[i].load(filepath);
		}
	}

	// Performe identity check and update the task
	int updateTaskWithData(JsonDocument & data, JsonDocument & response) {
		serializeJsonPretty(data, Serial);

		using namespace JsonKeys;
		int index = findTaskWithName(data[TASK_NAME]);
		if (index == -1) {
			response["error"] = "No task with such name";
			return -1;
		}

		// Check there is a task with newName then
		// replaces the name with new name if none is found
		if (data.containsKey(TASK_NEW_NAME)) {
			if (findTaskWithName(data[TASK_NEW_NAME]) == -1) {
				data[TASK_NAME] = data[TASK_NEW_NAME];
			} else {
				KPStringBuilder<64> error("Task with name ", data[TASK_NEW_NAME].as<char *>(), " already exist");
				response["error"] = (char *) error.c_str();
				return -1;
			}
		}

		JsonVariant payload = response.createNestedObject("payload");
		tasks[index]		= Task(data.as<JsonObject>());
		tasks[index].encodeJSON(payload);

		KPStringBuilder<100> success("Saved", data[TASK_NAME].as<char *>());
		response["success"] = (char *) success.c_str();
		return index;
	}

	// TODO: Server-side validation
	int validateTask(Task & task) {
		// Application & app = *static_cast<Application *>(controller);
		// task.schedule > now
		if (task.schedule < now() && task.schedule != -1) {
		}
		return 0;
	}

	// TODO: Server-side validation
	int validateTask(Task & task, JsonDocument & response) {
		return 0;
	}

	void setTaskStatus(int index, TaskStatus status) {
		tasks[index].status = status;
		updateObservers(&TaskObserver::taskDidUpdate, tasks[index]);
	}

	bool containsActiveTask() {
		return std::find_if(tasks.begin(), tasks.end(), [](const Task & t) {
			return t.status == TaskStatus::active;
		}) != tasks.end();
	}

	// Get next closest task with active status
	// This method mutates class member
	Task * nearestActiveTask() {
		// First we sort tasks according to their schedule time
		std::sort(tasks.begin(), tasks.end(), [](const Task & a, const Task & b) {
			return a.schedule < b.schedule;
		});

		// Then we partition the ones with active status to the left
		std::stable_partition(tasks.begin(), tasks.end(), [](const Task & a) {
			return a.status == TaskStatus::active;
		});

		// Return pointer to the first element or nulltptr if none
		return (tasks.empty() || tasks.front().status != TaskStatus::active) ? nullptr : &tasks.front();
	}

	// Create add a new task to task array and write to SD
	void createTask(const JsonObject & src) {
		Task task(src);
		task.schedule = now() - 1;
		tasks.push_back(task);
		writeTaskArrayToDirectory();
	}

	void add(const Task & task) {
		tasks.push_back(task);
		writeTaskArrayToDirectory();
	}

	void markTaskAsCompleted(Task & task) {
		task.markAsCompleted();
	}

	// ────────────────────────────────────────────────────────────────────────────────
	// Update the index file which has meta data about the tasks
	// ────────────────────────────────────────────────────────────────────────────────
	void updateIndexFile(const char * _dir = nullptr) {
		const char * dir = _dir ? _dir : taskFolder;

		JsonFileLoader loader;
		loader.createDirectoryIfNeeded(dir);

		KPStringBuilder<32> indexFilepath(dir, "/index.js");
		StaticJsonDocument<100> indexJson;
		indexJson["count"] = tasks.size();
		loader.save(indexFilepath, indexJson);
	}

	// Write task array to SD directory
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
		if (index < 0 || (size_t) index >= tasks.size()) {
			raise("TaskManager.deleteTask: Index out of range");
		}

		tasks.erase(tasks.begin() + index);
	}

	void cleanUpCompletedTasks() {
		auto predicate = std::remove_if(tasks.begin(), tasks.end(), [](const Task & task) {
			return task.status == TaskStatus::completed && task.deleteOnCompletion;	 // put your condition here
		});

		println("Size before: ", tasks.size());
		tasks.erase(predicate, tasks.end());
		println("Size after: ", tasks.size());
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
