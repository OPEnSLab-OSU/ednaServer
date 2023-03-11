#pragma once
#include<Utilities/LogObserver.hpp>

class Logger: public LogObserver {
    public:
        const char * logFilePath = nullptr;

    public:
        Logger() {}

        void init(Config & config) {
            logFilePath = config.printFile;
            if (!SD.exists(logFilePath)) {
                File file = SD.open(logFilePath, FILE_WRITE);
                file.close();
                Serial.println("debug file created!");
            }
        }

        void log(const String msg) override {
            Serial.println(logFilePath);
            File file = SD.open(logFilePath, FILE_WRITE);
            file.println(msg);
            file.close();
        }
};