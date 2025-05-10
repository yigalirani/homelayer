#pragma once
#include <vector>
#include <string>
#include <Windows.h>
using namespace std;
class Event {
public:
	unsigned char vcode;
	bool is_down;
	long long t;
	const char* comment = "";
};
class Alg {
public:
	virtual vector<Event> handle_event(Event& e) = 0;
	virtual ~Alg() {}
};
Alg* make_doubling_alg();
Alg* make_delay_alg();
Alg* make_pass_through();
Alg* make_delayed_down();

static const char* wmparm_to_tr(WPARAM parm);
static const char* is_down_to_tr(bool is_down);
using namespace std;
string to_hexstring(int x);
string vcode_to_string(int vcode);
string adjustString(const string& input, int length);
string pcode_to_str(Event e);
#define EPOCH_DIFFERENCE 11644473600000LL // Difference between 1601 and 1970 in milliseconds

long long get_cur_time();

vector<Event> loadEventsFromFile(const string& filename);
string get_dir(const Event& event);
void write_events(vector<Event> recorded_events);