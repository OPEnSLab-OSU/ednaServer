#pragma once
#include<SdFat.h>

class SDCard : public SdFat32 {
private:
    SDCard() {}
    ~SDCard() {}
    SDCard(SDCard const&) = delete;
    SDCard & operator = (SDCard const&) = delete;

public:
    using SdFat32::SdFat32;

	static SDCard & sharedInstance() {
		static SDCard SD;
		return SD;
	}
};