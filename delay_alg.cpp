#include "alg.hpp"


class DelayAlg:public Alg {
	vector<Event>handle_event(Event& e) {
		vector<Event> ans;
		ans.push_back(e);
		ans.push_back(e);
		return ans;
	}
};
Alg* make_delay_alg() {
	return new DelayAlg;
}