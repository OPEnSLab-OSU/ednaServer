#pragma once
#include <KPObserver.hpp>
class SensorArrayObserver : public KPObserver {
public:
	const char * ObserverName() const {
		return SensorManagerObserverName();
	}

	virtual const char * SensorManagerObserverName() const				 = 0;
	virtual void pressureSensorDidUpdate(std::pair<float, float> values) = 0;
};
