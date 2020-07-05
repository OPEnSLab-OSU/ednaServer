#include <Application/Application.hpp>
#include <array>

//
// ──────────────────────────────────────────────────────────────── I ──────────
//   :::::: S E R V E R   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────
//

void Application::setupServerRouting() {
	server.handlers.reserve(10);

#ifndef DEBUG
	// TODO: Respond with actual index file
	server.get("/", [this](Request & req, Response & res) {
		res.setHeader("Content-Encoding", "gzip");
		res.sendFile("index.gz", fileLoader);
		res.end();
	});
#endif

	// ────────────────────────────────────────────────────────────────────────────────
	// Get the current status
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/status", [this](Request &, Response & res) {
		StaticJsonDocument<Status::encodingSize()> response;
		encodeJSON(status, response.to<JsonObject>());
		response["utc"] = now();

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get a list of valve objects
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/valves", [this](Request &, Response & res) {
		StaticJsonDocument<ValveManager::encodingSize()> response;
		encodeJSON(vm, response.to<JsonArray>());

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get a list of task objects
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/tasks", [this](Request &, Response & res) {
		StaticJsonDocument<TaskManager::encodingSize()> response;
		encodeJSON(tm, response.to<JsonArray>());

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get task with name
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/get", [this](Request & req, Response & res) {
		StaticJsonDocument<Task::encodingSize()> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encodingSize() + 500> response;
		int id = body[TaskKeys::ID];

		if (tm.findTask(id)) {
			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[id], payload);
			response["success"] = "Task found";
		} else {
			response["error"] = "Task not found";
		}

		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Create a new task with name
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/create", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encodingSize() + 500> response;
		const char * name = body[TaskKeys::NAME];

		println("Creating Task..");

		// Adding new task to task collection and write to file +
		// sending http response as appropriate
		Task task = tm.createTask();
		strcpy(task.name, name);
		tm.insertTask(task, true);
		// tm.writeToDirectory(); // NOTE: Uncomment to save tasks

		// success response
		KPStringBuilder<100> success("Successfully created ", name);
		response["success"] = (char *) success.c_str();

		// return task with partially filled fields
		JsonVariant payload = response.createNestedObject("payload");
		encodeJSON(task, payload);

		res.json(response);
		res.end();
		println("Returned response");
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Update existing task with incoming data
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/save", [this](Request & req, Response & res) {
		StaticJsonDocument<Task::encodingSize()> body;
		deserializeJson(body, req.body);

#ifdef DEBUG
		serializeJsonPretty(body, Serial);
#endif

		Task incomingTask;
		incomingTask.decodeJSON(body.as<JsonVariant>());

		StaticJsonDocument<Task::encodingSize() + 500> response;
		validateTaskForSaving(incomingTask, response);
		if (response.containsKey("error")) {
			goto send;
		}

		tm.tasks[incomingTask.id] = incomingTask;
		tm.writeToDirectory();
		response["success"] = "Task successfully saved";

	send:
		serializeJsonPretty(response, Serial);
		res.json(response);
		res.end();
		println("Returned response");
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Schedule a task (marking it active)
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/schedule", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encodingSize() + 500> response;
		int id = body["name"];
		if (tm.findTask(id)) {
			//TODO: perform server validation
			if (tm.tasks[id].numberOfValves() == 0) {
				response["error"] = "Cannot schedule a task without an assigned valve";
				goto send;
			}

			// ...

			tm.setTaskStatus(id, TaskStatus::active);
			tm.writeToDirectory();
			scheduleNextActiveTask();

			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[id], payload);
			response["success"] = "Task has been scheduled";
		}

	send:
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Unschedule a task (making it inactive)
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/unschedule", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encodingSize() + 500> response;
		int id = body["id"];

		if (tm.findTask(id)) {
			Task & task = tm.tasks[id];
			if (currentTaskId == id) {
				invalidateTask(task);
				sm.transitionTo(StateName::STOP);
			} else {
				invalidateTask(task);
				tm.writeToDirectory();
			}

			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(task, payload);
			response["success"] = "Task is now inactive";
		} else {
			response["error"] = "No task with such name";
		}

		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Delete task with name
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/delete", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);

		StaticJsonDocument<500> response;
		int id = body["id"];

		if (!tm.findTask(id)) {
			response["error"] = "No task with this name";
			goto send;
		}

		if (tm.tasks[id].status == TaskStatus::active) {
			response["error"] = "Task currently have active status. Please deactivate before continue.";
			goto send;
		}

		tm.deleteTask(id);
		tm.writeToDirectory();
		response["success"] = "Task deleted";

	send:
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// RTC update
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/rtc/update", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);

		const time_t utc		 = body["utc"];
		const int timezoneOffset = body["timezoneOffset"];

		println("UTC=", utc);
		println("Compile Time=", power.compileTime(timezoneOffset));

		// Checking against compiled time + millis() to prevents bogus value
		StaticJsonDocument<100> response;
		if (utc >= power.compileTime(timezoneOffset) + millisToSecs(millis())) {
			response["success"] = "RTC updated";
			power.set(utc);
		} else {
			response["error"] = "That doesn't seem right. You sure about this?";
		}

		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Emergency stop
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/stop", [this](Request & req, Response & res) {
		sm.transitionTo(StateName::STOP);
	});
}

//
// ──────────────────────────────────────────────────────────────────── II ──────────
//   :::::: S E R I A L   C O M M A N D : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────────
//

class StringParser {
public:
	char buffer[256];
	std::array<KPString, 16> parts;

	StringParser(const char * line) {
		strcpy(buffer, line);
		parse();
	}

	void parse() {
		auto it = parts.begin();
		*it		= strtok(buffer, " ");
		while (it != parts.end()) {
			*(++it) = strtok(NULL, " ");
		}
	}
};

#define MatchLine(s) if (line == s)
#define Match(x, s)	 if (parts[x] == s)
#define HasValue(x)	 if (parts[x])
#define BeginParsing(line)             \
	StringParser parser(line.c_str()); \
	auto & parts = parser.parts;

void Application::commandReceived(const String & line) {
	JsonFileLoader loader;
	BeginParsing(line);

	Match(0, "test") {
		Match(1, "1") {
			println("It works");
		}
	}

	Match(1, "status") {
		Match(0, "read") {
			loader.load(config.statusFile, status);
		}

		Match(0, "save") {
			loader.save(config.statusFile, status);
		}

		Match(0, "print") {
			println(status);
		}

		return;
	}

	Match(1, "config") {
		Match(0, "load") {
			loader.load(config.configFilepath, config);
		}

		Match(0, "print") {
			println(config);
		}
	}

	Match(1, "sd") Match(0, "print") {
		printDirectory(SD.open("/"), 0);
	}

	Match(1, "valves") {
		Match(0, "load") {
			vm.loadValvesFromDirectory();
		}

		Match(0, "save") {
			vm.writeToDirectory();
		}

		Match(0, "print") {
			for (size_t i = 0; i < vm.valves.size(); i++) {
				println(vm.valves[i]);
			}
		}

		Match(0, "free") {
			if (parts[2]) {
				int valveNumber = atoi(parts[2]);
				if (valveNumber >= 0 && valveNumber < config.valveUpperBound) {
					vm.setValveStatus(valveNumber, ValveStatus::free);
				}
			} else {
				vm.init(config);
			}
		}

		Match(0, "reset") {
			for (int i = 0; i < config.numberOfValves; i++) {
				vm.setValveStatus(i, ValveStatus::Code(config.valves[i]));
			}

			vm.writeToDirectory();
		}
	}

	Match(0, "update") Match(1, "rtc") {
		power.set(power.compileTime());
	}

	Match(1, "tasks") {
		Match(0, "print") {
			println(tm);
		}

		Match(0, "save") {
			tm.writeToDirectory();
			tm.loadTasksFromDirectory();
			println(tm);
		}

		Match(0, "clear") {
			tm.tasks.clear();
			tm.updateIndexFile();
		}

		Match(0, "clean") {
			tm.deleteIf([](const Task & task) { return task.status == TaskStatus::completed; });
			tm.writeToDirectory();
		}
	}

	MatchLine("clear alarm") {
		println("Reseting alarm...");
		power.resetAlarms();
	}

	MatchLine("schedule") {
		println("Scheduling task...");
		scheduleNextActiveTask();
	}

	Match(0, "schedule") Match(1, "now") {
		println("Schduling temp task");
		Task task				= tm.createTask();
		task.schedule			= now() + 5;
		task.sampleTime			= 5;
		task.deleteOnCompletion = true;
		task.status				= TaskStatus::active;
		tm.insertTask(task);
		println(tm);
		ScheduleStatus s = scheduleNextActiveTask();
		println("Return Status: ", static_cast<int>(s));
	}

	MatchLine("mem") {
		println(free_ram());
	}
}
#undef match
#undef hasValue