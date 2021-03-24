#pragma once
#include <KPFoundation.hpp>
#include <Components/SDcard.hpp>
//#include <SdFat.h>

class FileLoader {
public:
    bool createDirectoryIfNeeded(const char * dir) {
        File32 folder = SDCard::sharedInstance().open(dir, FILE_READ);
        if (folder.isOpen()) {
            if (folder.isDirectory()) {
                folder.close();
                return true;
            }

            halt(TRACE, "Directory is a file. Please remove it.");
        }

        // folder doesn't exist
        print("FileLoader: ", dir, " directory doesn't exist. Creating...");
        bool success = SDCard::sharedInstance().mkdir(dir);
        println(success ? "success" : "failed");
        folder.close();
        return success;
    }
};