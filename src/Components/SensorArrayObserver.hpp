#pragma once
#include <KPObserver.hpp>
class SensorArrayObserver : public KPObserver {
public:
	const char * ObserverName() const {
		return SensorManagerObserverName();
	}

	virtual const char * SensorManagerObserverName() const = 0;
	virtual void pressureSensorDidUpdate(const std::pair<float, float> & values) {}
	virtual void baro1DidUpdate(const std::pair<float, float> & values) {}
	virtual void baro2DidUpdate(const std::pair<float, float> & values) {}
};
