#pragma once
class ValveStatus {
public:
	enum Code {
		unavailable = -1,
		sampled,	// 0
		free,		// 1
		operating,	// 2
		next,
		missed
	} _code;

	// Allow for implicit conversion from ValveStatus::code -> ValveStatus(ValveStatus::code)
	// Meaning that the client code can call function that accept ValveStatus with just ValveStatus::[enum]
	ValveStatus(Code code)
		: _code(code) {
	}

	Code code() const {
		return _code;
	}

	operator int() const {
		return _code;
	}
};