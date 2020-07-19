#include <KPController.hpp>
#include <StateControllers/HyperFlushStateController.hpp>

class TestApplication : public KPController {
public:
	HyperFlushStateController hyperFlushController;
	void setup() override {
		addComponent(hyperFlushController);
	}
} app;

void setup() {
	app.setup();
}

void loop() {
	app.update();
}