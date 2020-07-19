#ifndef UNIT_TEST
	#include <KPApplicationRuntime.hpp>
	#include <Application/Application.hpp>

void printDirectory(File dir, int numTabs) {
	while (true) {
		File entry = dir.openNextFile();
		if (!entry) {
			break;
		}

		for (uint8_t i = 0; i < numTabs; i++) {
			print('\t');
		}

		print(entry.name());

		if (entry.isDirectory()) {
			println("/");
			printDirectory(entry, numTabs + 1);
		} else {
			print("\t\t");
			println(entry.size(), DEC);	 // files have sizes, directories do not
		}

		entry.close();
	}
}

namespace {
	Application app;
}

void setup() {
	Runtime::setInitialAppController(app);
}

void loop() {
	Runtime::update();
}
#endif

// #include <KPFoundation.hpp>
// void setup() {
// 	Serial.begin(115200);
// 	while (!Serial) {}

// 	int i = 0;
// 	KPStringBuilder<32> test("valve-", i, ".js");
// 	println(test);
// }

// void loop() {}