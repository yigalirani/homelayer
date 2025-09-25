#include <Windows.h>
#include "alg.hpp"
#include <unordered_map>
#include <iostream>
#include "utils.hpp"

class AltKey {
public:
	unsigned char* layer_at_press=nullptr;//so can lookup replacement on up
	//long long last_press_time=0;
	//long long last_unpress_time=0;
};

void add_functions(unsigned char* ans)	{
	ans[VK_OEM_3] = VK_ESCAPE;
	ans[1] = VK_F1;
	ans[2] = VK_F2;
	ans[3] = VK_F3;
	ans[4] = VK_F4;
	ans[5] = VK_F5;
	ans[6] = VK_F6;
	ans[7] = VK_F7;
	ans[8] = VK_F8;
	ans[9] = VK_F9;
	ans[0] = VK_F10	;
	ans[VK_OEM_MINUS] = VK_F11;
	ans[VK_OEM_PLUS] = VK_F12;
}
unsigned char* make_left_alt_layer() {
	auto ans = make_layer();
	ans['H'] = VK_LEFT;
	ans['J'] = VK_DOWN;
	ans['K'] = VK_UP;
	ans['L'] = VK_RIGHT;

	ans['N'] = VK_HOME;
	ans['M'] = VK_END;


	ans['F'] = VK_LCONTROL;
	ans['D'] = VK_LSHIFT;
	ans['S'] = VK_LMENU;
	add_functions(ans);

	return ans;
}

unsigned char* make_right_alt_layer() {
	auto ans = make_layer();
	ans['J'] = VK_RCONTROL;
	ans['K'] = VK_RSHIFT;
	ans['L'] = VK_RMENU;
	add_functions(ans);
	return ans;
}

class AltAlg:public Alg {
	AltKey keys[256];
	unsigned char* cur_layer = nullptr;
	unsigned char* layers[256] = { nullptr };

public:
	AltAlg() {
		layers[VK_LMENU] = make_left_alt_layer();
		layers[VK_RMENU] = make_right_alt_layer();
	}
	vector<Event>handle_event(Event& e) {
		auto &key = keys[e.vcode];
		if (!e.is_down) {//it is up
			auto layer_at_press = key.layer_at_press;
			key.layer_at_press = nullptr;
			//key.last_unpress_time = e.t;
			if (layers[e.vcode] != nullptr) {
				cur_layer = nullptr;
				return {};
			}

			if (layer_at_press == nullptr)
				return { e };//unchanged
			auto replacment_code = layer_at_press[e.vcode];
			if (replacment_code == 0)
				return { e };
			return { {.vcode = replacment_code,.is_down = false,.t = e.t } };
		}
		//key.last_press_time = e.t;
		if (cur_layer == nullptr) {
			cur_layer = layers[e.vcode];
			if (cur_layer != nullptr)
				return {};//key is not sent becuase it was used to turn on the layer
			return{ e };//should clear the key[e.vocde]? or asset that is cleared
		}
		if (layers[e.vcode] != nullptr)
			return {}; //ignore it because keys that activate layers are not used to send keys ever
		key.layer_at_press = cur_layer;
		auto replacment_code = cur_layer[e.vcode];
		if (replacment_code == 0)
			return { e };
		return { { .vcode = replacment_code, .is_down = true, .t = e.t } };
	}
	~AltAlg() {

	}
};
Alg* make_alt_alg() {
	return new AltAlg;
}