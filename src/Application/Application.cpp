#include <Application/Application.hpp>

#include <array>

//
// ──────────────────────────────────────────────────────────────── I ──────────
//   :::::: S E R V E R   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────
//

void Application::setupServerRouting() {
	server.handlers.reserve(10);

	// TODO: Respond with actual index file
	// server.get("/", [this](Request & req, Response & res) {
	// 	res.setHeader("Content-Encoding", "gzip");
	// 	res.sendFile("index.gz", fileLoader);
	// 	res.end();
	// });

	server.get("/api/status", [this](Request & req, Response & res) {
		StaticJsonDocument<Status::encoderSize()> response;
		encodeJSON(status, response.to<JsonObject>());
		response["utc"] = now();

		// serializeJsonPretty(response, Serial);

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length.c_str());
		res.json(response);
		res.end();
	});

	// Get array of valves object
	server.get("/api/valves", [this](Request & req, Response & res) {
		StaticJsonDocument<ValveManager::encoderSize()> doc;
		encodeJSON(vm, doc.to<JsonArray>());

		KPStringBuilder<10> length(measureJson(doc));
		res.setHeader("Content-Length", length.c_str());
		res.json(doc);
		res.end();
	});

	// Get array of tasks obejects
	server.get("/api/tasks", [this](Request & req, Response & res) {
		StaticJsonDocument<TaskManager::encoderSize()> doc;
		encodeJSON(tm, doc.to<JsonArray>());
		res.json(doc);
		res.end();
	});

	// Get task with name
	server.post("/api/task/get", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		StaticJsonDocument<Task::encoderSize()> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);

		if (index == -1) {
			response["error"] = "Task not found";
		} else {
			response["success"] = "Task found";
			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[index], payload);
		}

		res.json(response);
		res.end();
	});

	// Create a new task with name
	server.post("/api/task/create", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);

		// Adding new task to task array and wrtie to file +
		// sending http response as appropriate
		if (index == -1) {
			tm.createTask(body.as<JsonObject>());
			tm.writeTaskArrayToDirectory();

			KPStringBuilder<100> success("Successfully created ", name);
			response["success"] = (char *) success.c_str();

			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks.back(), payload);
		} else {
			response["error"] = "Found task with the same name. Task name must be unique";
		}

		res.json(response);
		res.end();
	});

	// Update existing task with incoming data
	server.post("/api/task/save", [this](Request & req, Response & res) {
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		int index = tm.updateTaskWithData(body, response);
		if (index != -1) {
			println("Writing to directory");
			tm.writeTaskArrayToDirectory();
		}

		res.json(response);
		res.end();
	});

	// Put task on active status
	server.post("/api/task/schedule", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);
		if (index != -1) {
			if (tm.tasks[index].valveCount == 0) {
				response["error"] = "Cannot schedule a task without an assigned valve";
				goto send;
			}

			tm.markTask(index, TaskStatus::active);
			tm.writeTaskArrayToDirectory();

			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[index], payload);
			response["success"] = "Task has been scheduled";
		} else {
			response["error"] = "No Task with such name";
		}

	send:
		res.json(response);
		res.end();
	});

	server.post("/api/task/unschedule", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;

		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);
		if (index != -1) {
			tm.markTask(index, TaskStatus::inactive);
			tm.writeTaskArrayToDirectory();
			clearRemainingValves(tm.tasks[index]);
			tm.tasks[index].markAsCompleted();

			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[index], payload);
			response["success"] = "Task has been unscheduled";
		} else {
			response["error"] = "No Task with such name";
		}

		res.json(response);
		res.end();
	});

	// Delete task with name
	server.post("/api/task/delete", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);

		// Task not found
		if (index == -1) {
			response["error"] = "No task with this name";
			goto send;
		}

		// Found task but status is active
		if (tm.tasks[index].status) {
			response["error"] = "Task currently have active status. Please deactivate before continue.";
			goto send;
		}

		// If no error
		if (!response.containsKey("error")) {
			tm.deleteTask(index);
			tm.writeTaskArrayToDirectory();
			response["success"] = "Task deleted";
		}

	send:
		res.json(response);
		res.end();
	});

	// RTC update
	server.post("/api/rtc/update", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		const time_t utc		 = body["utc"];
		const int timezoneOffset = body["timezoneOffset"];

		println("utc=", utc);
		println("compileTime=", power.compileTime(timezoneOffset));

		StaticJsonDocument<100> response;
		if (utc >= power.compileTime(timezoneOffset) + (millis() / 1000)) {
			response["success"] = "RTC updated.";
			power.set(utc);
		} else {
			response["error"] = "Time seems sketchy. You sure about this?";
		}

		res.json(response);
		res.end();
	});

	// Emergency stop
	server.get("/stop", [this](Request & req, Response & res) {
		sm.transitionTo(StateName::STOP);
	});
}

//
// ──────────────────────────────────────────────────────────────────────────────── II ──────────
//   :::::: S E R I A L   C O M M A N D   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────────────────────
//

struct StringPart {
	StringPart * prev = nullptr;
	const char * ptr  = nullptr;

	void(*callback) = nullptr;

	StringPart()				   = default;
	StringPart(const StringPart &) = default;
	StringPart & operator=(const StringPart &) = default;

	StringPart(const char * _ptr)
		: ptr(_ptr) {}

	StringPart & operator=(const char * rhs) {
		ptr = rhs;
		return *this;
	}

	bool operator==(const char * rhs) const {
		return strcmp(ptr, rhs) == 0;
	}

	bool operator!=(const char * rhs) const {
		return !(*this == rhs);
	}

	operator const char *() const {
		return ptr;
	}
};

class StringParser {
public:
	char buffer[256];
	std::array<StringPart, 16> parts;
	std::array<StringPart, 16>::iterator it = parts.begin();
	size_t _size;

	StringParser(const char * line) {
		strcpy(buffer, line);
		parse();
		it = parts.begin();
	}

	void parse() {
		auto it	  = parts.begin();
		auto part = strtok(buffer, " ");
		*it		  = part;
		_size	  = 1;
		while (part && it != parts.end()) {
			*(++it)	 = strtok(NULL, " ");
			it->prev = it - 1;
			part	 = strtok(nullptr, " ");
			_size++;
		}
	}

	StringPart & current() {
		return *it;
	}

	StringPart & next() {
		return *(++it);
	}

	size_t size() const {
		return _size;
	}

	bool operator==(const char * rhs) {
		return *it == rhs;
	}

	bool operator!=(const char * rhs) {
		return *it != rhs;
	}
};

#define match(x, s) if (parts[x] == s)
#define hasValue(x) if (parts[x])
void Application::commandReceived(const String & line) {
	StringParser parser(line.c_str());
	auto & parts = parser.parts;

	match(0, "test") {
		match(1, "1") {
			println("It works");
		}
	}

	match(1, "status") {
		match(0, "read") {
			status.load(config.statusFile);
		}

		match(0, "save") {
			status.save(config.statusFile);
		}

		match(0, "print") {
			println(status);
		}

		return;
	}

	match(1, "config") {
		match(0, "load") {
			config.load();
		}

		match(0, "print") {
			println(config);
		}
	}

	match(1, "sd") match(0, "print") {
		printDirectory(SD.open("/"), 0);
	}

	match(1, "valves") {
		match(0, "load") {
			vm.loadValvesFromDirectory(config.valveFolder);
		}

		match(0, "save") {
			vm.saveValvesToDirectory(config.valveFolder);
		}

		match(0, "print") {
			for (size_t i = 0; i < vm.valves.size(); i++) {
				println(vm.valves[i]);
			}
		}

		match(0, "free") {
			if (parts[2]) {
				int valveNumber = atoi(parts[2]);
				if (valveNumber >= 0 && valveNumber < config.valveUpperBound) {
					vm.setValveStatus(valveNumber, ValveStatus::free);
				}
			} else {
				vm.init(config);
			}
		}
	}

	match(0, "update") match(1, "rtc") {
		power.set(power.compileTime());
	}

	match(1, "tasks") {
		match(0, "print") {
			println(tm);
		}

		match(0, "save") {
			tm.writeTaskArrayToDirectory();
			tm.loadTasksFromDirectory();
			println(tm);
		}
	}

	match(0, "mem") {
		println(free_ram());
	}
}
#undef match
#undef hasValue