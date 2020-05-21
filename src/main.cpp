#ifndef UNIT_TEST
	#include <KPApplicationRuntime.hpp>

	#include <Application/Application.hpp>
	#include <Application/Action.hpp>

void printDirectory(File dir, int numTabs) {
	while (true) {
		File entry = dir.openNextFile();
		if (!entry) {
			// no more files
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
			// files have sizes, directories do not
			print("\t\t");
			println(entry.size(), DEC);
		}
		entry.close();
	}
}

class TestApplication : public KPController {
public:
	void develop() {
		while (!Serial) {
			delay(100);
		};
	}

	void setup() override {
		Serial.begin(115200);
		develop();	// NOTE: Remove in production
		runForever(1000, "test", []() {
			println("TESTING...");
		});

		run(10000, []() {
			removeAction("test");
		});
	}
} test_app;

Application app;
void setup() {
	Runtime::setInitialAppController(app);
}

void loop() {
	ActionScheduler::sharedInstance().update();
	Runtime::update();
}
#endif