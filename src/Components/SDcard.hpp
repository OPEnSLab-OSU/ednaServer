#pragma once
#include<SdFat.h>

class SDCard{
private:
    SdFat sd;
    
    SDCard() {
    #ifdef DEBUG
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);
        pinMode(6, OUTPUT);
        digitalWrite(6, HIGH);
    #endif
    }
    
    ~SDCard() {
        //sd.end();
        //sd.errorHalt();
    }
    
    SDCard(SDCard const&) = delete;
    SDCard & operator = (SDCard const&) = delete;

public:
    bool begin(SdCsPin_t pin){

        return sd.begin(SdSpiConfig(pin, DEDICATED_SPI, SD_SCK_MHZ(12)));
        //return sd.begin(pin);
    }

    File open(const char *path, oflag_t flag=0X00){
        return sd.open(path, flag);
    }

    File open(const String &path, oflag_t flag=0X00){
        return sd.open(path, flag);
    }

    bool mkdir(const char *path, bool pFlag = true){
        return sd.mkdir(path, pFlag);
    }

    bool mkdir(const String &path, bool pFlag = true){
        return sd.mkdir(path, pFlag);
    }

    bool exists(const char *path){
        return sd.exists(path);
    }

    bool exists(const String &path){
        return sd.exists(path);
    }

    void end(){
        sd.end();
        return;
    }

    bool remove(const char *path){
        return sd.remove(path);
    }

    bool remove(const String &path){
        return sd.remove(path);
    }

    bool rename(const char *oldPath, const char *newPath){
        return sd.rename(oldPath, newPath);
    }

    bool rename(const String &oldPath, const String &newPath){
        return sd.rename(oldPath, newPath);
    }

    bool rmdir(const char *path){
        return sd.rmdir(path);
    }

    bool rmdir(const String &path){
        return sd.rmdir(path);
    }

    void initErrorPrint(){
        return sd.initErrorPrint();
    }

	static SDCard & sharedInstance() {
		static SDCard SDcard;
		return SDcard;
	}
};