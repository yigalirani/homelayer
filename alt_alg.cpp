#include <Windows.h>
#include "alg.hpp"
#include <unordered_map>
#include <iostream>
#include "utils.hpp"

class Key {
public:
	unsigned char* layer_at_press=nullptr;//so can lookup replacement on up
	long long last_press_time=0;
	long long last_unpress_time=0;
};


unsigned char* make_left_alt_layer() {
	auto ans = make_layer();
	ans['J'] = VK_UP;
	ans['M'] = VK_DOWN;
	ans['N'] = VK_LEFT;
	ans[VK_OEM_COMMA] = VK_RIGHT;
	ans['H'] = VK_HOME;
	ans['K'] = VK_END;
	ans['O'] = VK_PRIOR;
	ans['L'] = VK_NEXT;

	ans['F'] = VK_LCONTROL;
	ans['D'] = VK_LSHIFT;
	ans['S'] = VK_LMENU;

	return ans;
}

unsigned char* make_right_alt_layer() {
	auto ans = make_layer();
	ans['J'] = VK_RCONTROL;
	ans['K'] = VK_RSHIFT;
	ans['L'] = VK_RMENU;
	return ans;
}

class AltAlg:public Alg {
	Key keys[256];
	unsigned char* cur_layer = nullptr;
	unsigned char* layers[256] = { nullptr };

public:
	AltAlg() {
		layers[VK_LMENU] = make_left_alt_layer();
		layers[VK_RMENU] = make_right_alt_layer();
	}
	vector<Event>handle_event(Event& e) {
		auto key = keys[e.vcode];
		if (!e.is_down) {//it is up
			auto layer_at_press = key.layer_at_press;
			key.layer_at_press = nullptr;
			key.last_unpress_time = e.t;
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
		key.last_press_time = e.t;
		if (cur_layer == nullptr) {
			cur_layer = layers[e.vcode];
			if (cur_layer == nullptr)
				return {};//key is not sent becuase it was used to turn on the layer
			return{ e };//should clear the key[e.vocde]? or asset that is cleared
		}
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