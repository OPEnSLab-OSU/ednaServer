#pragma once
#define ARDUINOJSON_USE_LONG_LONG 1
#include <KPFoundation.hpp>
#include <ArduinoJson.h>
#include <tuple>
#include <Application/Status.hpp>

template <typename Signature>
struct APISpec;

template <typename _R, typename... Args>
struct APISpec<_R(Args...)> {
	using R			= _R;
	using ArgsTuple = std::tuple<Args...>;

	template <size_t I>
	using Arg = typename std::tuple_element<I, ArgsTuple>::type;
};

class App;
namespace API {
	struct StartHyperFlush : APISpec<StaticJsonDocument<300>(App &)> {
		auto operator()(Arg<0>) -> R;
	};

	struct StatusGet : APISpec<StaticJsonDocument<Status::encodingSize()>(App &)> {
		auto operator()(Arg<0>) -> R;
	};

	struct TaskCreate
		: APISpec<StaticJsonDocument<Task::encodingSize() + 500>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};

	struct TaskGet
		: APISpec<StaticJsonDocument<Task::encodingSize() + 500>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};

	struct TaskSave
		: APISpec<StaticJsonDocument<Task::encodingSize() + 500>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};

	struct TaskDelete : APISpec<StaticJsonDocument<500>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};

	struct TaskSchedule
		: APISpec<StaticJsonDocument<Task::encodingSize() + 500>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};

	struct TaskUnschedule
		: APISpec<StaticJsonDocument<Task::encodingSize() + 500>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};

	struct RTCUpdate : APISpec<StaticJsonDocument<100>(App &, JsonDocument &)> {
		auto operator()(Arg<0>, Arg<1>) -> R;
	};
};	// namespace API
