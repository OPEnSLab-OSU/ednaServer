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

	web.get("/api/tasks", [this](Request & req, Response & res) {
		StaticJsonDocument<TaskManager::encoderSize()> doc;
		tm.encodeJSON(doc.to<JsonArray>());
		res.json(doc);
		res.end();
	});

	web.post("/api/task/get", [this](Request & req, Response & res) {
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
			tm.tasks[index].encodeJSON(payload);
		}

		res.json(response);
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

		// Adding new task to task array and wrtie to file +
		// sending http response as appropriate
		if (index == -1) {
			tm.createTask(body.as<JsonObject>());
			tm.writeTaskArrayToDirectory();

			KPStringBuilder<100> success("Successfully created ", name);
			response["success"] = (char *) success.c_str();

			JsonVariant task = response.createNestedObject("payload");
			tm.tasks.back().encodeJSON(task);
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

		// TODO: Validate Task
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

	web.get("/stop", [this](Request & req, Response & res) {
		sm.transitionTo(StateName::STOP);
	});
}

//
// ──────────────────────────────────────────────────────────────────────────────── II ──────────
//   :::::: S E R I A L   C O M M A N D   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────────────────────
//

class InteractiveString {
public:
	static constexpr size_t size = 256;
	char buffer[size]{0};
	char * p = buffer;

	bool prefix(const char * pre) {
		if (p >= buffer + size) {
			return false;
		}

		const size_t len = strlen(pre);
		if (strncmp(pre, buffer, len) == 0) {
			p += len;
			return true;
		}

		return false;
	}
};

#define match(x)  if (line == x)
#define prefix(x) if (s.prefix(x))
#define next(x)
void Application::commandReceived(const String & line) {
	InteractiveString s;
	line.toCharArray(s.buffer, s.size);
	prefix("read") {
		println("COOL");
	}

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

	match("m") {
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

	match("read index") {
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