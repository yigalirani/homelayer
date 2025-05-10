#include <Windows.h>
#include "alg.hpp"
#include <unordered_map>
#include <iostream>
#include "utils.hpp"

class Key {
public:
	/*enum KeyStatus{
		idle = 0,
		pressed,
		canceled,//pressed before the elepsed time is needed?
		activataed 
	}status = idle;*/
	bool pressed = false;
	bool mod_key_sent = false;// ctl/shift/alt was already sent so dont send again and send up when the key is up
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


unsigned char* make_passthro() {
	unsigned char *ans= make_layer();
	ans[VK_MENU] = true;    // Alt key
	ans[VK_LMENU] = true;   // Left Alt
	ans[VK_RMENU] = true;   // Right Alt

	// Ctrl keys
	ans[VK_CONTROL] = true; // Ctrl key
	ans[VK_LCONTROL] = true; // Left Ctrl
	ans[VK_RCONTROL] = true; // Right Ctrl

	// Shift keys
	ans[VK_SHIFT] = true;   // Shift key
	ans[VK_LSHIFT] = true;  // Left Shift
	ans[VK_RSHIFT] = true;  // Right Shift

	// Windows keys
	ans[VK_LWIN] = true;    // Left Windows
	ans[VK_RWIN] = true;    // Right Windows
	return ans;
}
 
class DelayAlg:public Alg {
	unsigned char* layers[256]={ nullptr };
	Key keys[256];
	unsigned char* passthou = make_passthro();
	long long delay = 300;// in ms. todo: configurable
public:
	unsigned char get_subsitute(Event& e){
		auto vcode = e.vcode;
		for (int i = 0; i < 256; i++) {
			Key& key = keys[i];
			if (!key.pressed)
				continue;
			if ( key.last_press_time + delay > e.t)
				continue;
			cout << "layer in effect,last press time=" << key.last_press_time << "e.t=" << e.t << endl;
			auto layer = layers[i];
			if (layers[i] == nullptr)
				continue;
			auto ans = layer[vcode];
			if (ans != 0)
				return ans;
			return vcode;
		}
		return vcode;
	}
	DelayAlg() {
		layers[' '] = make_space_layer();
		print_layer(layers[' ']);
	}
	vector<Event>handle_event(Event& e) {
		if (passthou[e.vcode])
			return { e };
		vector<Event> ans;
		Key& key = keys[e.vcode];
		if (e.is_down) {
			key.pressed=true;
			key.last_press_time = get_cur_time();
			return ans;//todo add the logic for aut repeat:
		}
		key.pressed = false;//todo assert that that was true before
		auto vcode = get_subsitute(e);

		ans.push_back({ vcode ,true });
		ans.push_back({ vcode ,false });
		return ans;
	}
	~DelayAlg() {

	}
};
Alg* make_delay_alg() {
	return new DelayAlg;
}