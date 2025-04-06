#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>  
#include <set> 
#include "utils.hpp"
#include <cassert>
struct SendKey {
    int vcode;
    WPARAM wParam;
};
#define HOMELAYER_MAGIC 0xDEADBEEF

void send_key(vector<SendKey> keys) {
	vector<INPUT> inputs;
	for (auto key : keys) {
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = key.vcode;
		input.ki.wScan = 0;
		input.ki.dwFlags = (key.wParam == WM_KEYUP || key.wParam == WM_SYSKEYUP) ? KEYEVENTF_KEYUP : 0;
		input.ki.time = 0;
		input.ki.dwExtraInfo = HOMELAYER_MAGIC;
		inputs.push_back(input);
		cout << "\033[32m "<< pcode_to_str(key.wParam, key.vcode) << "\033[0m" << flush;
	}
	SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
}

bool is_up(WPARAM wParam) {
    return wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
}
bool is_down(WPARAM wParam) {
    return wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
}
class Key;

class StatefullKey;

class MainObject {
public:
    HHOOK hHook=0;
    Key** top_layer=nullptr;
    Key** cur_layer=nullptr;
    Key** active_keys = nullptr;
    StatefullKey*active_mods[4]; //when homerow mod on init stage needs to check in ignore if exists. clear on keyup
    Key* nop = nullptr;
}main_obj;
enum Mod {
    mod_shift = 0,
    mod_control,
    mod_alt,
    mod_layer1
};
enum KeyState {
    init = 0, //at the program start
    keydown, //after keydown
    keydown_elapsed,//after keydown and timeout
	realized//some non-mode key was pressed -send the aaprropriate keydown
};


const char* state_to_string(KeyState state) {
    switch (state) {
        case KeyState::init: return "init";
        case KeyState::keydown: return "keydown";
        case KeyState::keydown_elapsed: return "keydown_el";
        case KeyState::realized: return "realized";
        default: return "unknown";
    }
}

class Key {
public:
    virtual void event(WPARAM wParam,int vcode,long long event_time) = 0;
    virtual string get_full_name() = 0;
};
class NopKey :public Key {
public:
    string get_full_name() {
        return "NopKey";
    }
    void event(WPARAM wParam, int vcode, long long event_time) {
        cout << " ignored " << flush;
    }
};
#define TIMEOUT 200

void send_key_realize(vector<SendKey> keys, long long sender_keydown_time);
class StatefullKey :public Key {
protected:
    KeyState state = init ;
    Mod mod;
    string name;
    long long keydown_time = 0;
    void deactivate(long long event_time) {
        main_obj.active_mods[mod] = nullptr;
        on_deactivate(event_time);
        set_state(init);
    }
	void activate() {//used only once, but wanted symeticy with deactivate which is used multiple times
        set_state(keydown);
        main_obj.active_mods[mod] = this;
    }
    KeyState get_state(long long event_time) {
        if (state == keydown && event_time- keydown_time > TIMEOUT)
            set_state(keydown_elapsed);
        return state;
    }
    void set_state(KeyState _state) {
        cout  << "-->" << state_to_string(_state);
        state = _state;
    }


public:
    StatefullKey(const char* _name,Mod _mod) :name(_name), mod(_mod) {
    }  
    virtual void on_activate(WPARAM wParam, int vcode) = 0;
    virtual void on_deactivate(long long event_time) = 0;
    virtual int on_realize(long long sender_keydown_time) = 0;
    void event(WPARAM wParam, int vcode,long long event_time) {
        const auto state = get_state(event_time);
        switch (state) {
        case init:
            if (main_obj.active_mods[mod]) {
                send_key_realize({ {.vcode = vcode,.wParam = wParam} }, event_time); //do the default because the mirror homeromod is in effect
                return;
            }
            assert(!is_up(wParam));
            keydown_time = event_time;
			activate();
            on_activate(wParam, vcode);
            return;
        case keydown:
            if (is_up(wParam)) { //this means that we used the key as regular key which might have mods to realize
                //cout << "layer key up" << flush;
                deactivate(event_time);
                send_key_realize({
                    {.vcode = vcode ,.wParam = WM_KEYDOWN},
                    {.vcode = vcode,.wParam = WM_KEYUP}
                    }, keydown_time);

                return;
            }
            set_state(keydown_elapsed);
            return;
        case realized:
        case keydown_elapsed:
            if (is_up(wParam))
                return deactivate(event_time);
            //dont send nothing here because this key was functioned as mod
        }

    }
    string get_full_name() {
        return name + "(" +state_to_string(state)+ ")";

    }
};
void send_key_realize(vector<SendKey> keys,long long sender_keydown_time) {
    for (auto mod : main_obj.active_mods) {
        if (mod == nullptr)
            continue;
        int vcode = mod->on_realize(sender_keydown_time);
        if (vcode == -1)
            continue;
        const SendKey key = { .vcode = vcode,.wParam = WM_KEYDOWN };
        keys.insert(keys.begin(), key);
    }
	send_key(keys);
}
class ForwardKey:public Key {
public:
    int forwardvcode;
    ForwardKey(int _forwardvcode) :
        forwardvcode(_forwardvcode){
    }
    string get_full_name() {
        return "ForwardKey(" + vcode_to_string(forwardvcode) + ")";
    }
    void event(WPARAM wParam, int vcode,long long event_time) {
        send_key_realize({ { .vcode = forwardvcode,.wParam=wParam} },event_time);
    }
};
class Repeater :public Key {
public:
    string get_full_name() {
        return "Rep";
    }
    void event(WPARAM wParam, int vcode, long long event_time) {
        send_key_realize({ {.vcode = vcode,.wParam = wParam} }, event_time);
    }
};
class HomeRowKey :public StatefullKey {
public:
    HomeRowKey(Mod _mod) :StatefullKey("HomeRowKey", _mod) {
    }
    void on_activate(WPARAM wParam, int vcode) {
    }
    void on_deactivate(long long event_time) {
        if (get_state(event_time) == realized)
            send_key({
              {.vcode = mod + 16,.wParam = WM_KEYUP} //to match the keydown sent on_realize
                });
    }
	int on_realize(long long sender_keydown_time) {
        if (get_state(sender_keydown_time) == realized )
            return -1;
        auto diff = sender_keydown_time - keydown_time;
		cout << "diff" << diff << flush;
        if (diff > 0) {
			cout << "ret" << flush;
            return -1;
        }
        cout << "cont" << flush;
		if (diff < TIMEOUT) {
			send_key({
				{.vcode = mod + 16,.wParam = WM_KEYDOWN}
				});
		}   
		set_state(realized);
		return mod + 16; //because
	}

};

class LayerKey :public StatefullKey {
    Key** layer;
public:
    LayerKey(Key** _layer,Mod _mod) :layer(_layer), StatefullKey("LayerKey",_mod) {
    }
    void on_activate(WPARAM wParam, int vcode) {
        cout << "on nav" << flush;
        main_obj.cur_layer = layer;
    }
    void on_deactivate(long long event_time) {
        cout << "on top" << flush; 
        main_obj.cur_layer = main_obj.top_layer;
    }  
    int on_realize(long long sender_keydown_time) {
        return -1;
    }

};
Key** make_layer(Key *fill) {
    auto ans = new Key * [256];
    for (int i = 0; i < 255; i++)
        ans[i] = fill;
    return ans;
}
void add_left_mods(Key** layer) {
    layer['F'] = new HomeRowKey(mod_control );
    layer['D'] = new HomeRowKey(mod_shift);
    layer['S'] = new HomeRowKey(mod_alt);
}
void add_buildin_mods(Key** layer) {
    return; //not needed anymore. todo : delete
    layer[VK_LCONTROL] = new HomeRowKey(mod_control);
    layer[VK_LSHIFT] = new HomeRowKey(mod_shift);
    layer[VK_LMENU] = new HomeRowKey(mod_alt);
    layer[VK_RCONTROL] = new HomeRowKey(mod_control);
    layer[VK_RSHIFT] = new HomeRowKey(mod_shift);
    layer[VK_RMENU] = new HomeRowKey(mod_alt);
}

Key** make_nav_layer() {
    const auto ans = make_layer(nullptr);
    ans['J'] = new ForwardKey(VK_UP);
    ans['M'] = new ForwardKey(VK_DOWN);
    ans['N'] = new ForwardKey(VK_LEFT);
    ans[VK_OEM_COMMA] = new ForwardKey(VK_RIGHT);
    ans['H'] = new ForwardKey(VK_HOME);
    ans['K'] = new ForwardKey(VK_END);
    ans['O'] = new ForwardKey(VK_PRIOR);
    ans['L'] = new ForwardKey(VK_NEXT);
    //add_left_mods(ans);
    add_buildin_mods(ans);
    return ans;
}

Key** make_top_layer(){
    const auto ans = make_layer(new Repeater());
    //add_left_mods(ans);
    //ans['J'] = new HomeRowKey(mod_control);
    //ans['K'] = new HomeRowKey(mod_shift);
    //ans['L'] = new HomeRowKey(mod_alt);
    add_buildin_mods(ans);
    ans[' '] = new LayerKey(make_nav_layer(), mod_layer1);
    return ans;
}
void setup(){
    main_obj.top_layer = make_top_layer();
    main_obj.active_keys = make_layer(nullptr);
    main_obj.cur_layer = main_obj.top_layer;
    main_obj.nop = new NopKey();
}
/*alg:
* on f keydown: send ctrl on keydown
* on f keyup: send ctl keyup. state==keydown and passed less that thredhof. send fkeyown+fup
*/

// Callback function for the low-level keyboard hook
Key* get_key(int vcode) {
    auto ans = main_obj.active_keys[vcode];
    if (ans != nullptr)
        return ans;
    ans= main_obj.cur_layer[vcode];
    if (ans != nullptr)
        return ans;
     
    ans= main_obj.top_layer[vcode];
    if (ans != nullptr)
        return ans;

    if (main_obj.cur_layer != main_obj.top_layer)
        return main_obj.nop;
    return nullptr;
}      

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    KBDLLHOOKSTRUCT* pKbDllHookStruct = (KBDLLHOOKSTRUCT*)lParam;
    auto doit = [&]() {  
        if (nCode != HC_ACTION)             
            return false;
        if (pKbDllHookStruct->dwExtraInfo == HOMELAYER_MAGIC)
            return false; //skipped to avoid proccesing      our own synthetic 
        auto vcode = pKbDllHookStruct->vkCode; 
        auto key = get_key(vcode);
        if (is_up(wParam))
            main_obj.active_keys[vcode] = nullptr;
        else
            main_obj.active_keys[vcode] = key;

		assert(key);
        //if (key == nullptr) //todo: use the send key functiopn so realize function is called
         //   return false;
        const string start = adjustString(key->get_full_name(), 20) + " : ";
        std::cout << endl << start << "\033[93m " << pcode_to_str(wParam, vcode) << "\033[0m" << flush;
        key->event(wParam, vcode,get_cur_time());
        return true;
    }; 
    if (doit())
        return 1;
    return CallNextHookEx(main_obj.hHook, nCode, wParam, lParam);
}

// Function to install the hook
void InstallHook() {
    main_obj.hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (main_obj.hHook == NULL) {
        printf("Failed to install hook! Error: %lu\n", GetLastError());
        exit(EXIT_FAILURE);
    }
    else {
        printf("Hook installed successfully!\n");
    }
}

// Function to uninstall the hook
void UninstallHook() {
    if (main_obj.hHook != NULL) {
        UnhookWindowsHookEx(main_obj.hHook);
        printf("Hook uninstalled successfully!\n");
    }
}

int main() {
    MSG msg;
    setup();
    // Install the keyboard hook
    InstallHook();
    
    // Message loop to keep the program running
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Uninstall the keyboard hook
    UninstallHook();

    return 0;
}
