#include <Application/App.hpp>

//
// ──────────────────────────────────────────────────────────────── I ──────────
//   :::::: S E R V E R   S E T U P : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────
//

void sendJsonResponse(Response & res, JsonDocument & response) {
	res.json(response);
	res.end();
}

void App::setupServerRouting() {
	server.handlers.reserve(12);

	// server.get("/", [this](Request & req, Response & res) {
	// 	res.setHeader("Content-Encoding", "gzip");
	// 	res.sendFile("index.gz", fileLoader);
	// 	res.end();
	// });

	server.get("/api/preload", [this](Request &, Response & res) {
		const auto & response = dispatchAPI<API::StartHyperFlush>();
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Get the current status
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/api/status", [this](Request &, Response & res) {
		const auto & response = dispatchAPI<API::StatusGet>();

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

		const auto & response = dispatchAPI<API::TaskGet>(body);
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

		const auto & response = dispatchAPI<API::TaskCreate>(body);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Update existing task with incoming data
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/save", [this](Request & req, Response & res) {
		StaticJsonDocument<Task::encodingSize()> body;
		deserializeJson(body, req.body);
		serializeJsonPretty(body, Serial);

		const auto & response = dispatchAPI<API::TaskSave>(body);
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

		const auto & response = dispatchAPI<API::TaskSchedule>(body);
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

		const auto & response = dispatchAPI<API::TaskUnschedule>(body);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Delete task with name
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/task/delete", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);

		const auto & response = dispatchAPI<API::TaskDelete>(body);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// RTC update
	// ────────────────────────────────────────────────────────────────────────────────
	server.post("/api/rtc/update", [this](Request & req, Response & res) {
		StaticJsonDocument<100> body;
		deserializeJson(body, req.body);

		const auto & response = dispatchAPI<API::RTCUpdate>(body);
		res.json(response);
		res.end();
	});

	// ────────────────────────────────────────────────────────────────────────────────
	// Emergency stop
	// ────────────────────────────────────────────────────────────────────────────────
	server.get("/stop", [this](Request & req, Response & res) { newStateController.stop(); });
}

//
// ──────────────────────────────────────────────────────────────────── II ──────────
//   :::::: S E R I A L   C O M M A N D : :  :   :    :     :        :          :
// ──────────────────────────────────────────────────────────────────────────────
//



void App::commandReceived(const char * incomingMessage, size_t size) {
	KPString line{incomingMessage};

	println(line);

	if (incomingMessage[0] == '{') {
		StaticJsonDocument<255> doc;
		deserializeJson(doc, incomingMessage);

		StaticJsonDocument<255> response;
		// dispatch(doc["cmd"].as<const char *>(), doc, response);
	}

	if (line == "print status") {
		println(status);
	}

	if (line == "print config") {
		println(config);
	}

	if (line == "schedule now") {
		println("Schduling temp task");
		Task task				= tm.createTask();
		task.schedule			= now() + 5;
		task.flushTime			= 5;
		task.sampleTime			= 5;
		task.deleteOnCompletion = true;
		task.status				= TaskStatus::active;
		task.valves.push_back(0);
		tm.insertTask(task);
		println(scheduleNextActiveTask().description());
	}

	if (line == "reset valves") {
		for (int i = 0; i < config.numberOfValves; i++) {
			vm.setValveStatus(i, ValveStatus::Code(config.valves[i]));
		}

		vm.writeToDirectory();
	}

	if (line == "mem") {
		println(free_ram());
	}

	if (line == "hyperflush") {
		beginHyperFlush();
	};
}