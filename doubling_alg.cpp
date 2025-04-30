#include "alg.hpp"


class DoublingAlg:public Alg {
	vector<Event>handle_event(Event& e) {
		vector<Event> ans;
		ans.push_back(e);
		ans.push_back(e);
		return ans;
	}
};
Alg* make_doubling_alg() {
	return new DoublingAlg;
}