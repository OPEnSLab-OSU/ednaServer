#pragma once
#include<SdFat.h>
//#include<FatLib/FatVolume.h>

//static SdFat32 SD;

class SDCard : public SdFs {
private:
	SdFs SDcard;

public:
    using SdFs::SdFs;
    //using FatVolume::FatVolume;
    //SdFat32& get_SD_card() {
    //    return SDcard;
    //}
    //void open(const char * d, oflag_t oflag = O_RDONLY){

    //}
	static SDCard & sharedInstance() {
		static SDCard SD;
		return SD;
	}
};