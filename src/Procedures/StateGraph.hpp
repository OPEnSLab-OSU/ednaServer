#include <KPState.hpp>


class StateNode : public KPState {
public:
	KPString nextStateName;
	KPStateMachine * sm = nullptr;
	StateNode(KPString nextStateName) 
	: nextStateName(nextStateName) {}

	void enter(KPStateMachine & sm) override {
		this->sm = &sm;
	}

	void next() {
		
	}
};
