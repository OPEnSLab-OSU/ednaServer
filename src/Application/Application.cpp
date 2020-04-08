#include <Application/Application.hpp>

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

	web.get("/", [this](Request & req, Response & res) {
		res.setHeader("Content-Encoding", "gzip");
		res.sendFile("index.gz", fileLoader);
		res.end();
	});

	web.get("/api/status", [this](Request & req, Response & res) {
		const int size = ProgramSettings::STATUS_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> doc;
		JsonVariant object = doc.to<JsonObject>();
		status.encodeJSON(object);

		KPStringBuilder<10> length(measureJson(doc));

		res.setHeader("Content-Type", "application/json");
		res.setHeader("Content-Length", length.c_str());
		res.json(doc);
		res.end();
	});

	web.get("/api/valverefs", [this](Request & req, Response & res) {
		const size_t size = ProgramSettings::VALVEREF_JSON_BUFFER_SIZE * ProgramSettings::MAX_VALVES;
		StaticJsonDocument<size> doc;
		JsonArray valverefs = doc.to<JsonArray>();
		for (const Valve & v : vm.valves) {
			JsonVariant obj = valverefs.createNestedObject();
			Valveref ref(v);
			ref.encodeJSON(obj);
			serializeJsonPretty(obj, Serial);
		}

		res.setHeader("Content-Type", "application/json");
		res.json(doc);
		res.end();
	});

	web.get("/api/taskrefs", [this](Request & req, Response & res) {
		const size_t size = ProgramSettings::TASKREF_JSON_BUFFER_SIZE * 10;
		StaticJsonDocument<size> doc;
		JsonArray taskrefs = doc.to<JsonArray>();
		for (const Task & task : tm.tasks) {
			JsonVariant obj = taskrefs.createNestedObject();
			Taskref ref(task);
			ref.encodeJSON(obj);
			serializeJsonPretty(obj, Serial);
		}

		res.setHeader("Content-Type", "application/json");
		res.json(doc);
		res.end();
	});

	web.post("/api/task/get", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> doc;
		deserializeJson(doc, req.body);
		serializeJsonPretty(doc, Serial);

		// Find task with name
		int index = tm.findTaskWithName(doc[TASK_NAME]);
		if (index == -1) {
			res.send("{}");
			res.end();
			return;
		}

		StaticJsonDocument<size> task_doc;
		JsonVariant task_object = task_doc.to<JsonObject>();
		tm.tasks[index].encodeJSON(task_object);
		res.setHeader("Content-Type", "application/json");
		res.json(task_doc);
		res.end();
	});

	web.post("/api/task/create", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		const char * taskname = body["name"].as<char *>();
		const auto found	  = std::find_if(tm.tasks.begin(), tm.tasks.end(),
			 [&](Task t) {
				 return strcmp(t.name, taskname) == 0;
			 });

		// Not found. Can create new task.
		StaticJsonDocument<ProgramSettings::TASK_JSON_BUFFER_SIZE * 10 + 500> response;
		if (found == tm.tasks.end()) {
			JsonVariant task_array = response.createNestedArray("payload");

			tm.createTask(body.as<JsonObjectConst>());
			tm.encodeJSON(task_array);
			// tm.saveTasksToDirectory();

			KPStringBuilder<100> successMessage("Successfully created ", taskname);
			response["success"] = successMessage.c_str();
		} else {
			response["error"] = "Found task with the same name. Task name must be unique";
		}

		res.setHeader("Content-Type", "application/json");
		res.json(response);
		res.end();
	});

	web.post("/api/task/save", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		StaticJsonDocument<ProgramSettings::TASK_JSON_BUFFER_SIZE + 500> response;
		tm.updateTaskWithData(body, response);
		res.setHeader("Content-Type", "application/json");
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
		task.decodeJSON(body.as<JsonObjectConst>());

		int code = tm.validateTask(task);

		println(task);
		res.end();

		// StaticJsonDocument<ProgramSettings::TASK_JSON_BUFFER_SIZE + 500> response;
		// tm.updateTaskWithData(body, response);
		// res.setHeader("Content-Type", "application/json");
		// res.json(response);
		// res.end();

		// int index = tm.findTaskWithName(body[TASK_NAME]);
		// if (index == -1) {
		// 	res.send("{}");
		// 	res.end();
		// 	return;
		// }

		// tm.tasks[index] = Task(body.as<JsonObjectConst>());
		// StaticJsonDocument<ProgramSettings::TASK_JSON_BUFFER_SIZE * 10> doc;
		// JsonVariant task_array = doc.to<JsonArray>();
		// tm.encodeJSON(task_array);
		// res.setHeader("Content-Type", "application/json");
		// res.json(doc);
		// res.end();
		// tm.saveTasksToDirectory();
	});

	web.post("/api/task/schedule/forced", [this](Request & req, Response & res) {
		using namespace JsonKeys;
		const size_t size = ProgramSettings::TASK_JSON_BUFFER_SIZE;
		StaticJsonDocument<size> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);
	});

	web.get("/stop", [this](Request & req, Response & res) {
		sm.transitionTo(StateName::FLUSH);
	});

	web.post("/submit", [this](Request & req, Response & res) {

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

	match("read test") {
		char buffer[64];
		while (fileLoader.loadContentOfFile("index.gz", buffer, 64)) {
			println(buffer);
		}
	}

	match("test") {
		KPArray<int, ProgramSettings::MAX_VALVES> task1 = vm.filter([](const Valve & v) {
			return strcmp(v.group, "Task 1") == 0;
		});

		for (size_t i = 0; i < task1.size(); i++) {
			println(task1[i], ",");
		}
	}
}
#undef match