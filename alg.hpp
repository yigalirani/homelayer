#pragma once
#include <vector>
using namespace std;
class Event {
public:
	unsigned char vcode;
	bool is_down;
	long long t;
};
class Alg {
public:
	virtual vector<Event> handle_event(Event& e) = 0;
	virtual ~Alg() {}
};
Alg* make_doubling_alg();