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
        } else {
            if(string_to_tpic(args[1]) != -1){
                shift.setPin(string_to_tpic(args[1]), HIGH);
            }
        }
        shift.write();
    };

    mapNameToCallback["close"] = [this](std::vector<std::string> args) {
        if(is_number(args[1])){
            for(unsigned int i = 1; i < args.size(); i++){
                shift.setPin( std::stoi(args[i]) + shift.capacityPerRegister, LOW);
            }
        } else {
            if(string_to_tpic(args[1]) != -1){
                shift.setPin(string_to_tpic(args[1]), LOW);
            }
        }
        shift.write();
    };

    mapNameToCallback["pump"] = [this](std::vector<std::string> args) {
        const char * endpoint = args[1].c_str();
        if (strcmp(endpoint, "on") == 0) {
            pump.on();
        }
        if (strcmp(endpoint, "off") == 0) {
            pump.off();
        }
    };

    mapNameToCallback["intake"] = [this](std::vector<std::string> args){
        const char * endpoint = args[1].c_str();
        if(strcmp(endpoint, "on") == 0){
            intake.on();
        }
        if(strcmp(endpoint, "off") == 0){
            intake.off();
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

        if(strcmp(endpoint, "time") == 0) {
            power.printCurrentTime();
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
            return;
        }
    };

    mapNameToCallback["alarm"] = [this](std::vector<std::string> args) {
        long time = std::labs(std::stol(args[1]));
        power.scheduleNextAlarm(time + now());
        return;
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
}