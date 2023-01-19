#ifdef COMPONENT_TEST
#include <Application/App.hpp>
#include <Components/Sensors/FlowSensor.hpp>

namespace {

    using func = std::function<void(std::vector<std::string> args)>;
    using StringToFunc = std::unordered_map<std::string, func>;
    StringToFunc mapNameToCallback;

    constexpr const char EOT = '\4';

    void endTransmission() {
        println(EOT);
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

    void ListCommands(){
        println("Available serial commands: ");
        println("open [list of ints] - open a list of filter valves");
        println("open [air/alcohol/flush] - open said valve");
        println("close [list of ints] - close a list of filter valves");
        println("close [air/alcohol/flush] - close said valve");
        println("pump [on/off/reverse] - change state of pump");
        println("intake [on/off] - change state of intake valve");
        println("query [status/config/ram/time/sensors] - print info about these objects");
        println("alarm [int] - schedule alarm interrupt n seconds in the future (will print info but do nothing)");
        return;
    }
}  // namespace

// Registering Top-level serial commands
void App::setupSerialRouting() {
    mapNameToCallback["help"] = [this](std::vector<std::string> args) {
        ListCommands();
        return;
    };

    mapNameToCallback["open"] = [this](std::vector<std::string> args) {
        //assume that it is a list of valveIDs
        if(is_number(args[1])){
            for(unsigned int i = 1; i < args.size(); i++){
                shift.setPin( std::stoi(args[i]) + shift.capacityPerRegister, HIGH); // adding capacityPerRegister skips the first register
            }
        } else {
            if(string_to_tpic(args[1]) != -1){
                shift.setPin(string_to_tpic(args[1]), HIGH);
            }
        }
        shift.write();
        return;
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
        return;
    };

    mapNameToCallback["pump"] = [this](std::vector<std::string> args) {
        const char * endpoint = args[1].c_str();
        if (strcmp(endpoint, "on") == 0) {
            pump.on();
        }
        if (strcmp(endpoint, "reverse") == 0) {
            pump.on(Direction::reverse);
        }
        if (strcmp(endpoint, "off") == 0) {
            pump.off();
        }
        return;
    };

    mapNameToCallback["intake"] = [this](std::vector<std::string> args){
        const char * endpoint = args[1].c_str();
        if(strcmp(endpoint, "on") == 0){
            intake.on();
        }
        if(strcmp(endpoint, "off") == 0){
            intake.off();
        }
        return;
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

    // This function is pointless in component testing mode because you can't sample in the mode
    // Keeping the code commented out in case this changes in the future

/*  mapNameToCallback["reset"] = [this](std::vector<std::string> args) {
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
    }; */
}

void App::commandReceived(const char * msg, size_t size) {

    std::vector<std::string> args;
    char str[80];
    strcpy(str, msg);
    println("");
    const char delim[2] = " ";
    const char * tok	= strtok(str, delim);
    while (tok != NULL) {
        args.push_back(tok);
        tok		= strtok(NULL, delim);
    }
    for(auto & command : mapNameToCallback) {
        if(command.first == args[0]){
            command.second(args);
            return;
        }
    }
    //if input doesn't match any commands
    ListCommands();
    return;
}
#endif