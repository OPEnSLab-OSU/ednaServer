#pragma once
#include <vector>
#include "Application/Observer.hpp"
#include "Task.hpp"

class TaskObserver : public Observer {
public:
	static const char * ObserverName() {
		return "TaskListner";
	}

	virtual void taskDidUpdate(const Task & task)						  = 0;
	virtual void taskCollectionDidUpdate(const std::vector<Task> & tasks) = 0;
};