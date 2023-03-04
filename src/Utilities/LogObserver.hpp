#pragma once
#include <KPObserver.hpp>
#include <Task/NowTask.hpp>
#include <vector>

class LogObserver : public KPObserver {
public:
    const char * ObserverName() const {
        return LogObserverName();
    }

    virtual const char * LogObserverName() const {
        return "<Unnamed> Log Observer";
    }

    virtual void log(const String msg)     = 0;
};