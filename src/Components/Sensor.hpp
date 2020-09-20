#pragma once
#include <KPFoundation.hpp>
#include <functional>
#include <tuple>

#include <Wire.h>

template <typename Function, typename Tuple, size_t... I>
auto call(Function f, Tuple t, std::index_sequence<I...>) {
	return f(std::get<I>(t)...);
}

template <typename Function, typename Tuple>
auto call(Function f, Tuple t) {
	static constexpr auto size = std::tuple_size<Tuple>::value;
	return call(f, t, std::make_index_sequence<size>{});
}

template <typename First, typename... Types>
class Sensor {
public:
	struct ErrorCode {
		enum Code { success = 0, tooEarly, invalidChecksum } _code;
		ErrorCode(Code code) : _code(code) {}

		operator Code() const {
			return _code;
		}
	};

private:
	ErrorCode errorCode			 = ErrorCode::success;
	unsigned long updateInterval = 0;

public:
	using SensorData = const First;
	std::function<void(SensorData)> onReceived;

	virtual void begin()	  = 0;	// setting the sensor
	virtual SensorData read() = 0;

	void setErrorCode(ErrorCode code) {
		errorCode = code;
	}

	ErrorCode getErrorCode() {
		return errorCode;
	}

	auto setUpdateFreq(double freqHz) {
		if (freqHz <= 0) {
			updateInterval = INT64_MAX;
		} else {
			updateInterval = static_cast<int>(1000.0 / freqHz);
		}
	}

	virtual ErrorCode update() final {
		static long last_update = millis();
		if ((millis() - last_update) < updateInterval) {
			return ErrorCode::tooEarly;
		}

		setErrorCode(ErrorCode::success);
		const auto response	 = read();
		const auto errorCode = getErrorCode();
		last_update			 = millis();

		if (errorCode == ErrorCode::success && onReceived) {
			onReceived(response);
		}

		return errorCode;
	}
};

template <typename First, typename Second, typename... Types>
struct Sensor<First, Second, Types...> : public Sensor<std::tuple<First, Second, Types...>> {};