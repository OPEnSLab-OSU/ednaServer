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
			tm.markTask(index, TaskStatus::active);
			tm.writeTaskArrayToDirectory();

			JsonVariant payload = response.createNestedObject("payload");
			encodeJSON(tm.tasks[index], payload);
			response["success"] = "Task has been scheduled";
		} else {
			response["error"] = "No Task with such name";
		}

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

class StringParser {
public:
	const char * parts[10];
};

#define match(x) if (line == x)
void Application::commandReceived(const String & line) {
	match("read status") {
		status.load(config.statusFile);
	}

	match("save status") {
		status.save(config.statusFile);
	}

	match("print status") {
		println(status);
	}

	match("load config") {
		config.load();
	}
	match("print config") {
		println(config);
	}

	match("print sd") {
		printDirectory(SD.open("/"), 0);
	}

	match("load valves") {
		vm.loadValvesFromDirectory(config.valveFolder);
	}

	match("save valves") {
		vm.saveValvesToDirectory(config.valveFolder);
	}

	match("print valves") {
		for (size_t i = 0; i < vm.valves.size(); i++) {
			println(vm.valves[i]);
		}
	}

	match("reset valves") {
		for (int i = 0; i < config.valveUpperBound; i++) {
			vm.init(config);
		}
	}

	match("update rtc") {
		power.set(power.compileTime());
	}

	match("print tasks") {
		println(tm);
	}

	match("save tasks") {
		tm.writeTaskArrayToDirectory();
		tm.loadTasksFromDirectory();
		println(tm);
	}

	match("read test") {
		char buffer[64];
		while (fileLoader.loadContentOfFile("index.gz", buffer)) {
			println(buffer);
		}
	}

	match("mem") {
		println(free_ram());
	}
}

#ifdef match
	#undef match
#endif