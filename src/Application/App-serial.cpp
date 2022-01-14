#include <Application/App.hpp>
#include <Components/Sensors/FlowSensor.hpp>

namespace {

    using func = std::function<void(std::vector<std::string> args)>;
    struct func_args {
        func function;
        unsigned short n_args;
    };
    struct SerialRequest {
        const char * cmd;
        const JsonArray & path;
        const JsonDocument & input;
    };

    using FunctionType = std::function<void(SerialRequest req)>;
    using StringToFunc = std::unordered_map<std::string, func>;
    StringToFunc mapNameToCallback;

    constexpr const char EOT = '\4';

    void endTransmission() {
        print(EOT);
    }

    // https://stackoverflow.com/questions/4654636/how-to-determine-if-a-string-is-a-number-with-c
    bool is_number(const std::string& s)
    {
        std::string::const_iterator it = s.begin();
        while (it != s.end() && std::isdigit(*it)) ++it;
        return !s.empty() && it == s.end();
    }

    int string_to_tpic(const std::string& s){
        if(s == "air"){
            return TPICDevices::AIR_VALVE;
        }
        if(s == "alcohol"){
            return TPICDevices::ALCHOHOL_VALVE;
        }
        if(s == "flush"){
            return TPICDevices::FLUSH_VALVE;
        }
        return -1;
    }
}  // namespace

// Registering Top-level serial commands
void App::setupSerialRouting() {
    mapNameToCallback["open"] = [this](std::vector<std::string> args) {
        //assume that it is a list of valveIDs
        if(is_number(args[1])){
            for(unsigned int i = 1; i < args.size(); i++){
                shift.setPin( std::stoi(args[i]) + shift.capacityPerRegister, HIGH);
            }
            shift.write();
        } else {
            if(string_to_tpic(args[1]) != -1){
                shift.setPin(string_to_tpic(args[1]), HIGH);
            }
        }
    };

    mapNameToCallback["close"] = [this](std::vector<std::string> args) {
        if(is_number(args[1])){
            for(unsigned int i = 1; i < args.size(); i++){
                shift.setPin( std::stoi(args[i]) + shift.capacityPerRegister, LOW);
            }
            shift.write();
        } else {
            if(string_to_tpic(args[1]) != -1){
                shift.setPin(string_to_tpic(args[1]), LOW);
            }
        }
    };

    mapNameToCallback["pump"] = [this](std::vector<std::string> args) {
        const char * endpoint = args[1].c_str();
        if (strcmp(endpoint, "on") == 0) {
            pump.on();
        }
        if (strcmp(endpoint, "status") == 0) {
            pump.off();
        }
    };

    mapNameToCallback["query"] = [this](std::vector<std::string> args) {
        const char * endpoint = args[1].c_str();
        if (strcmp(endpoint, "status") == 0) {
            const auto & response = dispatchAPI<API::StatusGet>();
            serializeJson(response, Serial);
            endTransmission();
            return;
        }

        if (strcmp(endpoint, "config") == 0) {
            const auto & response = dispatchAPI<API::ConfigGet>();
            serializeJson(response, Serial);
            endTransmission();
            return;
        }

        if (strcmp(endpoint, "ram") == 0) {
            printFreeRam();
            endTransmission();
            return;
        }

        if (strcmp(endpoint, "sensors") == 0) {
            if(sensors.pressure.enabled){
                println("Pressure sensor detected");
            } else {
                println(RED("Pressure sensor not detected"));
            }
            if(sensors.baro1.enabled){
                println("Baro1 sensor detected");
            }else{
                println(RED("Baro1 sensor not detected"));
            }
            if(sensors.baro2.enabled){
                println("Baro2 sensor detected");
            }else{
                println(RED("Baro2 sensor not detected"));
            }
        }
    };

    mapNameToCallback["reset"] = [this](std::vector<std::string> args) {
        const char * endpoint = args[1].c_str();
        if (strcmp(endpoint, "valves") == 0) {
            for (int i = 0; i < config.numberOfValves; i++) {
                vm.setValveStatus(i, ValveStatus::Code(config.valves[i]));
            }

            vm.writeToDirectory();

            StaticJsonDocument<ValveManager::encodingSize()> response;
            encodeJSON(vm, response.to<JsonVariant>());
            serializeJson(response, Serial);
            endTransmission();
            return;
        }
    };
}

/**
 *
 * {
 * 	path: ["top-level command", "sub1", "sub2"],
 * cmd: ""
 * 	...
 * }
 */

void App::commandReceived(const char * msg, size_t size) {

    std::vector<std::string> args;
		char str[80];
		strcpy(str, msg);
		println("Received: ", str);
		const char delim[2] = " ";
		const char * tok	= strtok(str, delim);
		int i				= 0;
		while (tok != NULL) {
			args.push_back(tok);
			tok		= strtok(NULL, delim);
			++i;
		}
        for(auto & command : mapNameToCallback) {
            if(command.first == args[0]){
                command.second({args});
                break;
            }
        }

    //if (msg[0] == '{') {
    //    StaticJsonDocument<512> input;
    //    deserializeJson(input, msg);

    //    for (auto & item : mapNameToCallback) {
    //        if (item.first == input["cmd"].as<const char *>()) {
    //            item.second({input["cmd"], input["path"], input});
    //            break;
    //        }
    //    }
    //}

    // if (strcmp(msg, "flow temp")) {
    // 	// auto response = FlowSensor::read();
    // }

    // if (strcmp(msg, "flow flow")) {}
    // KPString line{msg};

    // println(line);

    // if (msg[0] == '{') {
    // 	StaticJsonDocument<255> doc;
    // 	deserializeJson(doc, msg);

    // 	StaticJsonDocument<255> response;
    // 	// dispatch(doc["cmd"].as<const char *>(), doc, response);
    // }

    //if (strcmp(msg, "status")) {
    //    println(status);
    //}

    // if (line == "print config") {
    // 	println(config);
    // }

    // if (line == "schedule now") {
    // 	println("Schduling temp task");
    // 	Task task				= tm.createTask();
    // 	task.schedule			= now() + 5;
    // 	task.flushTime			= 5;
    // 	task.sampleTime			= 5;
    // 	task.deleteOnCompletion = true;
    // 	task.status				= TaskStatus::active;
    // 	task.valves.push_back(0);
    // 	tm.insertTask(task);
    // 	println(scheduleNextActiveTask().description());
    // }

    //if (strcmp(msg, "reset valves") == 0) {
    //    for (int i = 0; i < config.numberOfValves; i++) {
    //        vm.setValveStatus(i, ValveStatus::Code(config.valves[i]));
    //    }
    //}

    // 	vm.writeToDirectory();
    // }

    // if (line == "mem") {
    // 	println(free_ram());
    // }

    // if (line == "hyperflush") {
    // 	beginHyperFlush();
    // };
}