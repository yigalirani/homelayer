#include <Windows.h>
#include "alg.hpp"
#include <unordered_map>
#include <iostream>
class Key {
public:
	enum KeyStatus {
		idle = 0,
		pressed,
		canceled,//pressed before the elepsed time is needed?
		activataed // ctl/shift/alt was already sent
	}status = idle;
	long long last_press_time=0;
	long long last_unpress_time=0;
};


unsigned char* make_layer() {
	auto ans= new unsigned char[256];
	for (int i = 0; i < 256; i++)
		ans[i] = 0;
	return ans;
}
void print_layer(unsigned char*layer) {
	for (int i = 0; i < 256; i++)
		if (layer[i] != 0)
			cout << (char)i << " " << layer[i] << endl;
	

}
unsigned char* make_space_layer() {
	auto ans = make_layer();
	ans['J'] = VK_UP;
	ans['M'] = VK_DOWN;
	ans['N'] = VK_LEFT;
	ans[VK_OEM_COMMA] = VK_RIGHT;
	ans['H'] = VK_HOME;
	ans['K'] = VK_END;
	ans['O'] = VK_PRIOR;
	ans['L'] = VK_NEXT;
	return ans;
}


class DelayAlg:public Alg {
	unordered_map<unsigned char, unsigned char *> layers;
	Key keys[256];
public:
	DelayAlg() {
		layers[' '] = make_space_layer();
		print_layer(layers[' ']);
	}
	vector<Event>handle_event(Event& e) {
		vector<Event> ans;
		ans.push_back(e);
		ans.push_back(e);
		return ans;
	}
	~DelayAlg() {

	}
};
Alg* make_delay_alg() {
	return new DelayAlg;
}