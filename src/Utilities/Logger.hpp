#pragma once
#include<Utilities/LogObserver.hpp>

class Logger: public LogObserver {
    public:
        const char * logFilePath;

    public:
            Logger(const char * logFilePath)
        : logFilePath(logFilePath) {}

    void log(const String msg) override {
        char *cstr = new char[msg.length() + 1];
        strcpy(cstr, msg.c_str());

        File file = SD.open(logFilePath, FILE_WRITE);
		file.write(cstr, msg.length() + 1);
		file.close();
        delete [] cstr;
    }
};