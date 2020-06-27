
// References:
// https://embeddedartistry.com/blog/2017/02/01/improving-your-callback-game/
// https://en.cppreference.com/w/cpp/language/parameter_pack
template <typename T>
class Subject {
protected:
	using ObserverType = T;
	std::unordered_map<long, ObserverType *> observers;

public:
	int addObserver(ObserverType & o) {
		int token = rand();
		observers.insert({token, &o});
		return token;
	}

	void removeObserver(long token) {
		observers.erase(token);
	}

	template <typename F, typename... Types>
	void updateObservers(F method, Types... args) {
		println("Updating ", ObserverType::ObserverName());
		for (auto & k : observers) {
			std::bind(method, k.second, args...)();
		}
	}
};