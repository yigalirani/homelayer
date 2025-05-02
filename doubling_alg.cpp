#include "alg.hpp"


class DoublingAlg:public Alg {
	vector<Event>handle_event(Event& e) {
		vector<Event> ans;
		ans.push_back(e);
		ans.push_back(e);
		return ans;
	}
};
class pass_through :public Alg {
	vector<Event>handle_event(Event& e) {
		vector<Event> ans;
		ans.push_back(e);
		return ans;
	}
};
class delayed_down :public Alg {
	vector<Event>handle_event(Event& e) {
		vector<Event> ans;
		if (!e.is_down) {
			ans.push_back({ e.vcode, true });
			ans.push_back({ e.vcode, false });
		}
		return ans;
	}
};

Alg* make_doubling_alg() {
	return new DoublingAlg;
}
Alg* make_pass_through() {
	return new pass_through;
}
Alg* make_delayed_down() {
	return new delayed_down;
}