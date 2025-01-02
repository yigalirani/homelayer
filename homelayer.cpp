#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>  
#include <set> 
#include "utils.hpp"
#include <cassert>
struct SendKey {
    int vcode;
    int wParam;
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
	}
	SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
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
    virtual void event(WPARAM wParam,int vcode) = 0;
    virtual string get_full_name() = 0;
};
class NopKey :public Key {
public:
    string get_full_name() {
        return "NopKey";
    }
    void event(WPARAM wParam, int vcode) {
        cout << " ignored " << flush;
    }
};
#define TIMEOUT 200

void send_key_realize(vector<SendKey> keys);
class StatefullKey :public Key {
protected:
    KeyState state = init ;
    Mod mod;
    string name;

    long long last_update = 0;
    long long mark_time() {
        long long cur_time= get_cur_time();
        long long ans = cur_time - last_update;
        last_update = cur_time;
        return ans;
    } 
    void deactivate() {
        main_obj.active_mods[mod] = nullptr;
        set_state(init);
    }
    void activate() {
        set_state(keydown);
        main_obj.active_mods[mod] = this;
    }
    KeyState get_state() {
        long long elapsed = mark_time();
        if (state == keydown && elapsed > TIMEOUT)
            set_state(keydown_elapsed);
        return state;
    }
    void set_state(KeyState _state) {
        cout  << "-->" << state_to_string(_state);
        state = _state;
    }
    bool try_to_activate(WPARAM wParam, int vcode) {
        if (main_obj.active_mods[mod]) {
            send_key({ {.vcode = vcode,.wParam = WM_KEYUP} }); //do the default because the mirror homeromod is in effect
            return false;
        }
        assert(!is_up(wParam));
        activate();
		return true;
    }
public:
    StatefullKey(const char* _name,Mod _mod) :name(_name,mod) {
    }  
    string get_full_name() {
        return name + "(" +state_to_string(state)+ ")";

    }
    virtual int realize() = 0;
};
void send_key_realize(vector<SendKey> keys) {
    for (auto mod : main_obj.active_mods) {
        if (mod == nullptr)
            continue;
        int vcode = mod->realize();
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
    void event(WPARAM wParam, int vcode) {
        send_key({ { .vcode = forwardvcode,.wParam=wParam} });
    }
};
class HomeRowKey:public StatefullKey {
public:
    HomeRowKey(Mod _mod):StatefullKey("HomeRowKey",_mod){
    }               
    void event(WPARAM wParam, int vcode) {
        const auto state = get_state();
        switch (state) {
        case init:
            try_to_activate(wParam, vcode);
            return; //dont send the key for now.

        case keydown:
            assert(main_obj.active_mods[mod] == this);
            if (is_up(wParam)) {
                deactivate();
                send_key_realize({
                    {.vcode = vcode ,.wParam = WM_KEYDOWN},
                    {.vcode = vcode,.wParam = WM_KEYUP}
                    });
                return;
            }
            set_state(keydown_elapsed); //where keyup does not send nothing
            return;
        case realized:
        case keydown_elapsed: //do nothing
            if (is_up(wParam))
                return deactivate();
            return;
        }
    }
};

class LayerKey :public StatefullKey {
    Key** layer;
public:
    LayerKey(Key** _layer) :layer(_layer), StatefullKey("LayerKey") {
    }
    void event(WPARAM wParam, int vcode) {
        const auto state = get_state();
        switch (state) {
        case init:
            if (try_to_activate(wParam, vcode))
                main_obj.cur_layer = layer;
            return;
        case keydown:
            if (is_up(wParam)) { //this means that we used the key as regular key which might have mods to realize
                //cout << "layer key up" << flush;
                send_key_realize({
                    {.vcode = vcode ,.wParam = WM_KEYDOWN},
                    {.vcode = vcode,.wParam = WM_KEYUP}
                    });
                deactivate();
                main_obj.cur_layer = main_obj.top_layer;
                return;
            }
            set_state(keydown_elapsed);
            return;
        case realized:
        case keydown_elapsed: //do nothing
            if (is_up(wParam))
                return deactivate();
        }

    }

};
Key** make_layer() {
    auto ans = new Key * [256];
    for (int i = 0; i < 255; i++)
        ans[i] = nullptr;
    return ans;
}
void add_left_mods(Key** layer) {
    layer['F'] = new HomeRowKey(mod_control );
    layer['D'] = new HomeRowKey(mod_shift);
    layer['S'] = new HomeRowKey(mod_alt);
}
void add_buildin_mods(Key** layer) {
    layer[VK_LCONTROL] = new HomeRowKey(mod_control);
    layer[VK_LSHIFT] = new HomeRowKey(mod_shift);
    layer[VK_LMENU] = new HomeRowKey(mod_alt);
    layer[VK_RCONTROL] = new HomeRowKey(mod_control);
    layer[VK_RSHIFT] = new HomeRowKey(mod_shift);
    layer[VK_RMENU] = new HomeRowKey(mod_alt);
}

Key** make_nav_layer() {
    const auto ans = make_layer();
    ans['J'] = new ForwardKey(VK_UP);
    ans['M'] = new ForwardKey(VK_DOWN);
    ans['N'] = new ForwardKey(VK_LEFT);
    ans[VK_OEM_COMMA] = new ForwardKey(VK_RIGHT);
    add_left_mods(ans);
    add_buildin_mods(ans);
    return ans;
}

Key** make_top_layer(){
    const auto ans = make_layer();
    add_left_mods(ans);
    ans['J'] = new HomeRowKey(mod_control);
    ans['K'] = new HomeRowKey(mod_shift);
    ans['L'] = new HomeRowKey(mod_alt);
    add_buildin_mods(ans);
    ans[' '] = new LayerKey(make_nav_layer());
    return ans;
}
void setup(){
    main_obj.top_layer = make_top_layer();
    main_obj.cur_layer = main_obj.top_layer;
    main_obj.nop = new NopKey();
}
/*alg:
* on f keydown: send ctrl on keydown
* on f keyup: send ctl keyup. state==keydown and passed less that thredhof. send fkeyown+fup
*/

// Callback function for the low-level keyboard hook
Key* get_key(int vcode) {
    auto ans= main_obj.cur_layer[vcode];
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
        if (key == nullptr)
            return false;
        const string start = adjustString(key->get_full_name(), 20) + " : ";
        std::cout << endl << start << pcode_to_str(wParam, vcode) << flush;
        key->event(wParam, vcode);
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
