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

	// ────────────────────────────────────────────────────────────────────────────────
	// Get the current status
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/status", [this](Request & req, Response & res) {
		StaticJsonDocument<Status::encoderSize()> response;
		encodeJSON(status, response.to<JsonObject>());
		response["utc"] = now();

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length.c_str());
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get a list of valve objects
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/valves", [this](Request & req, Response & res) {
		StaticJsonDocument<ValveManager::encoderSize()> response;
		encodeJSON(vm, response.to<JsonArray>());

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get a lit of task objects
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/tasks", [this](Request & req, Response & res) {
		StaticJsonDocument<TaskManager::encoderSize()> response;
		encodeJSON(tm, response.to<JsonArray>());
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get task with name
	// ────────────────────────────────────────────────────────────────────────────────
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
			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[index], payload);
			response["success"] = "Task found";
		}

		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Create a new task with name
	// ────────────────────────────────────────────────────────────────────────────────
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
			Task new_task(body.as<JsonObject>());
			new_task.schedule = now() - 1;
			tm.insert(new_task);
			tm.writeTaskArrayToDirectory();

			// success response
			KPStringBuilder<100> success("Successfully created ", name);
			response["success"] = (char *) success.c_str();

			// return task with partially filled fields
			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks.back(), payload);
		} else {
			response["error"] = "Found task with the same name. Task name must be unique";
		}

		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Update existing task with incoming data
	// ────────────────────────────────────────────────────────────────────────────────
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

	// ────────────────────────────────────────────────────────────────────────────────
	// Schedule a task (marking it active)
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/schedule", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);
		if (index != -1) {
			//TODO: perform server validation
			if (tm.tasks[index].numberOfValves() == 0) {
				response["error"] = "Cannot schedule a task without an assigned valve";
				goto send;
			}

			tm.setTaskStatus(index, TaskStatus::active);
			tm.writeTaskArrayToDirectory();
			scheduleNextActiveTask();

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

	// ────────────────────────────────────────────────────────────────────────────────
	// Unschedule a task (making it inactive)
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/unschedule", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);

		if (index != -1) {
			tm.setTaskStatus(index, TaskStatus::inactive);
			tm.writeTaskArrayToDirectory();
			invalidateValvesForTask(tm.tasks[index]);
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

	// ────────────────────────────────────────────────────────────────────────────────
	// Delete task with name
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/delete", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);

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
			tm.removeTask(index);
			tm.writeTaskArrayToDirectory();
			response["success"] = "Task deleted";
		}

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

		println("utc=", utc);
		println("compileTime=", power.compileTime(timezoneOffset));

		// Checking against compiled time + millis() (which should be behind UTC) prevents bogus value
		StaticJsonDocument<100> response;
		if (utc >= power.compileTime(timezoneOffset) + millisToSecs(millis())) {
			response["success"] = "RTC updated";
			power.set(utc);
		} else {
			response["error"] = "That doens't seem right. You sure about this?";
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
			vm.writeValvesToDirectory();
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
	}

	Match(0, "update") Match(1, "rtc") {
		power.set(power.compileTime());
	}

	Match(1, "tasks") {
		Match(0, "print") {
			println(tm);
		}

		Match(0, "save") {
			tm.writeTaskArrayToDirectory();
			tm.loadTasksFromDirectory();
			println(tm);
		}

		Match(0, "clear") {
			tm.tasks.clear();
			tm.updateIndexFile();
		}

		Match(0, "clean") {
			tm.removeIf([](const Task & task) { return task.status == TaskStatus::completed; });
			tm.writeTaskArrayToDirectory();
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
		Task task				= tm.createTask();
		task.schedule			= now() + 5;
		task.sampleTime			= 5;
		task.deleteOnCompletion = true;
		task.status				= TaskStatus::active;
		tm.insert(task);
		scheduleNextActiveTask();
	}

	MatchLine("mem") {
		println(free_ram());
	}
}
#undef match
#undef hasValue