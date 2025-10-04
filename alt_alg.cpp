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
unsigned char* make_alt_layer() {
	int size = 256 * 2;
	auto ans = new unsigned char[size];
	for (int i = 0; i < size; i++)
		ans[i] = 0;
	return ans;
}
void add_functions(unsigned char* ans)	{
	ans[VK_OEM_3*2] = VK_ESCAPE;
	ans['1'*2] = VK_F1;
	ans['2'*2] = VK_F2;
	ans['3'*2] = VK_F3;
	ans['4' * 2] = VK_F4;
	ans['5' * 2] = VK_F5;
	ans['6' * 2] = VK_F6;
	ans['7' * 2] = VK_F7;
	ans['8' * 2] = VK_F8;
	ans['9' * 2] = VK_F9;
	ans['0' * 2] = VK_F10	;
	ans[VK_OEM_MINUS * 2] = VK_F11;
	ans[VK_OEM_PLUS * 2] = VK_F12;
}
unsigned char* make_left_alt_layer() {
	auto ans = make_alt_layer();
	ans['H' * 2] = VK_LEFT;
	ans['J' * 2] = VK_DOWN;
	ans['K' * 2] = VK_UP;
	ans['L' * 2] = VK_RIGHT;

	ans['N' * 2] = VK_HOME;
	ans['M' * 2] = VK_END;


	ans['F' * 2] = VK_LCONTROL;
	ans['D' * 2] = VK_LSHIFT;
	ans['S' * 2] = VK_LMENU;

	ans['C' * 2] = VK_LCONTROL;
	ans['C' * 2+1] = 'C';

	ans['X' * 2] = VK_LCONTROL;
	ans['X' * 2 + 1] = 'X';

	ans['V' * 2] = VK_LCONTROL;
	ans['V' * 2 + 1] = 'V';
	add_functions(ans);

	return ans;
}

unsigned char* make_right_alt_layer() {
	auto ans = make_alt_layer();
	ans['J' * 2] = VK_RCONTROL;
	ans['K' * 2] = VK_RSHIFT;
	ans['L' * 2] = VK_RMENU;
	add_functions(ans);
	return ans;
}
vector<Event>make_reponse(unsigned char* layer, Event  e) {
	vector<Event> ans;
	auto replacment_code = layer[e.vcode * 2];
	if (replacment_code == 0)
		return { e }; //uncacnged
	if (layer[e.vcode * 2 + 1] == 0)
		return { {.vcode = layer[e.vcode * 2], .is_down = e.is_down , .t = e.t } };
	for (int i = 0; i < 2; i++) {
		auto replacment_code = layer[e.vcode * 2 + i];
		ans.push_back({ .vcode = replacment_code, .is_down = e.is_down , .t = e.t });
	}
	if (!e.is_down)
		std::reverse(ans.begin(), ans.end());
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
			return make_reponse(layer_at_press, e);
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
			return make_reponse(cur_layer, e);
	}
	~AltAlg() {

	}
};
Alg* make_alt_alg() {
	return new AltAlg;
}