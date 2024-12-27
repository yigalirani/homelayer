#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>  
#include <set> 
#include "utils.hpp"
#define HOMELAYER_MAGIC 0xDEADBEEF
void send_key(WPARAM wParam, int vcode) {
    INPUT input;
    input.type = INPUT_KEYBOARD; 
    input.ki.wVk = vcode;
    input.ki.wScan = 0;
    input.ki.dwFlags = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ? KEYEVENTF_KEYUP : 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = HOMELAYER_MAGIC;
    cout << ">  "<<pcode_to_str(wParam, vcode) << flush;
    SendInput(1, &input, sizeof(INPUT));
}
bool is_up(WPARAM wParam) {
    return wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
}
bool is_down(WPARAM wParam) {
    return wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
}
class Key;


class MainObject {
public:
    HHOOK hHook=0;
    Key** top_layer=nullptr;
    Key** cur_layer=nullptr;
    std::set<int> locked_mods; //when homerow mod on init stage needs to check in ignore if exists. clear on keyup
    std::vector<Key*> active_keys;
    Key* nop = nullptr;
}main_obj;

enum KeyState {
    init = 0, //at the program start
    keydown, //after keydown
    locked,//after some keydown
};
const char* state_to_string(KeyState state) {
    switch (state) {
        case KeyState::init: return "init";
        case KeyState::keydown: return "keydown";
        case KeyState::locked: return "locked";
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
class StatefullKey :public Key {
    KeyState state = init;
protected: 
    string name;
    KeyState get_state() {
        return state;
    }
    void set_state(KeyState _state) {
        cout  << "-->" << state_to_string(_state);
        state = _state;
    }
public:
    StatefullKey(const char* _name) :name(_name) {
    }  
    string get_full_name() {
        return name + "(" +state_to_string(state)+ ")";

    }

};

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
        send_key(wParam,forwardvcode);
    }
};
class HomeRowKey:public StatefullKey {
    int modevcode;
public:
    HomeRowKey(int _modevcode):modevcode(_modevcode), StatefullKey("HomeRowKey"){
    }               
    void event(WPARAM wParam, int vcode) {
        if (get_state()== init) {
            if (main_obj.locked_mods.count(modevcode))
                return send_key(wParam, vcode); //do the default because the mirror homeromod is in effect
            if (is_up(wParam)) {
                printf("unexpexted state for homerow"); //hepfule will next get here
                return send_key(wParam, vcode);
            }   
            set_state(keydown);
            main_obj.locked_mods.insert(modevcode);
            return send_key(wParam, modevcode);
        }
        if (get_state()==keydown) {
            if (is_up(wParam)) {
                send_key(WM_KEYUP, modevcode);
                send_key(WM_KEYDOWN, vcode); //revert the mod
                send_key(WM_KEYUP, vcode);
                main_obj.locked_mods.erase(modevcode);
                set_state(init);
                return;
            }
            send_key(WM_KEYDOWN, modevcode); //double dowm on the mode as 
            set_state(locked);
            return;
        }
        if (get_state() == locked) {
            if (is_up(wParam)) {
                send_key(WM_KEYUP, modevcode); //revert the mod
                //dont send the original key because its bein too long
                main_obj.locked_mods.erase(modevcode);
                set_state(init);
                return;
            }
            send_key(WM_KEYDOWN, modevcode); //double dowm on the mode as 
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
        if (get_state() == init) {
            if (is_up(wParam)) {
                printf("unexpexted state for LayerKey"); //hepfule will not get here
                return send_key(wParam, vcode);
            }
            main_obj.cur_layer = layer;
            set_state(keydown);
            return;
        } 
        if (get_state() == keydown) {
            if (is_up(wParam)) {
                //cout << "layer key up" << flush;
                send_key(WM_KEYDOWN, vcode);
                send_key(WM_KEYUP, vcode);
                set_state(init);
                main_obj.cur_layer = main_obj.top_layer;
                return;
            }
            set_state(locked);
            return;
        }
        //send_key(wParam, vcode);
        if (is_up(wParam)) {
            main_obj.cur_layer = main_obj.top_layer;
            set_state(init);
            return;
        }
        //nothing to do on key down

    }

};
Key** make_layer() {
    auto ans = new Key * [256];
    for (int i = 0; i < 255; i++)
        ans[i] = nullptr;
    return ans;
}
void add_left_mods(Key** layer) {
    layer['F'] = new HomeRowKey(VK_CONTROL);
    layer['D'] = new HomeRowKey(VK_SHIFT);
    layer['S'] = new HomeRowKey(VK_MENU);
}
void add_buildin_mods(Key** layer) {
    layer[VK_LCONTROL] = new HomeRowKey(VK_CONTROL);
    layer[VK_LSHIFT] = new HomeRowKey(VK_SHIFT);
    layer[VK_LMENU] = new HomeRowKey(VK_MENU);
    layer[VK_RCONTROL] = new HomeRowKey(VK_CONTROL);
    layer[VK_RSHIFT] = new HomeRowKey(VK_SHIFT);
    layer[VK_RMENU] = new HomeRowKey(VK_MENU);
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
    ans['J'] = new HomeRowKey(VK_CONTROL);
    ans['K'] = new HomeRowKey(VK_SHIFT);
    ans['L'] = new HomeRowKey(VK_MENU);
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
