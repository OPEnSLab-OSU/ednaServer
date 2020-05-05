#pragma once
class TaskStatus {
public:
	enum Code {
		inactive,	// 0
		active,		// 1
		completed,	// 2
		missed		// 3
	} _code;

	// Allow for implicit conversion from TaskStatus::code -> TaskStatus(TaskStatus::code)
	// Meaning that the client code can call function that accept ValveStatus with just ValveStatus::[enum]
	TaskStatus(Code code)
		: _code(code) {
	}

	Code code() const {
		return _code;
	}

	operator int() const {
		return _code;
	}
};