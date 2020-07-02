#pragma once
#include <KPObserver.hpp>
#include <Task/Task.hpp>
#include <vector>

class TaskObserver : public KPObserver {
public:
	static const char * ObserverName() {
		return "TaskListner";
	}

	virtual void taskDidUpdate(const Task & task)						  = 0;
	virtual void taskCollectionDidUpdate(const std::vector<Task> & tasks) = 0;
	virtual void taskDidDelete(int id)									  = 0;
};