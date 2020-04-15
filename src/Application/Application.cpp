#include <Application/Application.hpp>

#include <array>

//
// ──────────────────────────────────────────────────────────────── I ──────────
//   :::::: S E R V E R   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────
//

void Application::setupServer() {
	// web.get("/", [this](Request & req, Response & res) {
	// 	res.sendFile("combined.htm", fileLoader);
	// 	res.end();
	// });

	web.handlers.reserve(10);

	web.get("/", [this](Request & req, Response & res) {
		res.setHeader("Content-Encoding", "gzip");
		res.sendFile("index.gz", fileLoader);
		res.end();
	});

	web.get("/api/status", [this](Request & req, Response & res) {
		StaticJsonDocument<Status::encoderSize()> response;
		status.encodeJSON(response.to<JsonObject>());
		response["utc"] = now();

		serializeJsonPretty(response, Serial);

		KPStringBuilder<10> length(measureJson(response));
		res.setHeader("Content-Length", length.c_str());
		res.json(response);
		res.end();
	});

	web.post("/api/rtc/update", [this](Request & req, Response & res) {
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

	web.get("/api/valves", [this](Request & req, Response & res) {
		StaticJsonDocument<ValveManager::encoderSize()> doc;
		vm.encodeJSON(doc.to<JsonArray>());

		KPStringBuilder<10> length(measureJson(doc));
		res.setHeader("Content-Length", length.c_str());
		res.json(doc);
		res.end();
	});

	// web.get("/api/taskrefs", [this](Request & req, Response & res) {
	// 	const size_t size = ProgramSettings::TASKREF_JSON_BUFFER_SIZE * 10;
	// 	StaticJsonDocument<size> doc;
	// 	JsonArray taskrefs = doc.to<JsonArray>();
	// 	for (const Task & task : tm.tasks) {
	// 		JsonVariant obj = taskrefs.createNestedObject();
	// 		Taskref ref(task);
	// 		ref.encodeJSON(obj);
	// 		serializeJsonPretty(obj, Serial);
	// 	}

	// 	res.setHeader("Content-Type", "application/json");
	// 	res.json(doc);
	// 	res.end();
	// });

	web.get("/api/tasks", [this](Request & req, Response & res) {
		StaticJsonDocument<TaskManager::encoderSize()> doc;
		tm.encodeJSON(doc.to<JsonArray>());
		res.json(doc);
		res.end();
	});

	web.post("/api/task/get", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		StaticJsonDocument<Task::encoderSize()> doc;
		deserializeJson(doc, req.body);
		serializeJsonPretty(doc, Serial);
		println();

		// Find task with name
		int index = tm.findTaskWithName(doc[TASK_NAME]);
		if (index == -1) {
			res.send("{}");
			res.end();
			return;
		}

		StaticJsonDocument<Task::encoderSize()> task_doc;
		JsonVariant task_object = task_doc.to<JsonObject>();
		tm.tasks[index].encodeJSON(task_object);
		res.json(task_doc);
		res.end();
	});

	web.post("/api/task/create", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		const char * name = body["name"];
		const int index	  = tm.findTaskWithName(name);
		if (index == -1) {
			tm.createTask(body.as<JsonObject>());
			tm.writeTaskArrayToDirectory();

			JsonVariant task = response.createNestedObject("payload");
			tm.tasks.back().encodeJSON(task);

			KPStringBuilder<100> successMessage("Successfully created ", name);
			response["success"] = (char *) successMessage.c_str();
		} else {
			response["error"] = "Found task with the same name. Task name must be unique";
		}

		res.json(response);
		res.end();
	});

	web.post("/api/task/save", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<Task::encoderSize() + 500> response;
		tm.updateTaskWithData(body, response);
		res.json(response);
		res.end();
	});

	web.post("/api/task/schedule", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		Task task;
		task.decodeJSON(body.as<JsonObject>());

		// tm.validateTask();

		StaticJsonDocument<Task::encoderSize() + 500> response;
		tm.updateTaskWithData(body, response);
		res.json(response);
		res.end();
	});

	web.post("/api/task/schedule/forced", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);
	});

	web.post("/api/task/delete", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		const char * name = body["name"];

		StaticJsonDocument<500> response;
		int index = tm.findTaskWithName(name);

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

	web.get("/stop", [this](Request & req, Response & res) {
		sm.transitionTo(StateName::STOP);
	});
}

//
// ──────────────────────────────────────────────────────────────────────────────── II ──────────
//   :::::: S E R I A L   C O M M A N D   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────────────────────
//

#define match(x) if (line == x)
void Application::commandReceived(const String & line) {
	match("load status") status.load(config.statusFile);
	match("save status") status.save(config.statusFile);
	match("print status") println(status);

	match("load config") config.load();
	match("print config") println(config);

	match("print sd") printDirectory(SD.open("/"), 0);

	match("load valves") vm.loadValvesFromDirectory(config.valveFolder);
	match("save valves") vm.saveValvesToDirectory(config.valveFolder);
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

	match("test") {
		auto valves_in_task1 = vm.filter([](const Valve & v) {
			return strcmp(v.group, "Task 1") == 0;
		});

		for (const auto & v : valves_in_task1) {
			println(v);
		}
	}

	match("index") {
		char buffer[64];
		fileLoader.loadContentOfFile("tasks/index.js", buffer);
		println(buffer);
	}

	if (line.startsWith("time")) {
		const char * str = line.c_str() + line.indexOf('e') + 1;
		int time		 = std::atoi(str);
		println(time);
		power.setTimeout(time, true);
	}
}

#ifdef match
#	undef match
#endif