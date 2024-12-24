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
    cout << " , "<<pcode_to_str(wParam, vcode) << flush;
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
}main_obj;

enum KeyState {
    init = 0, //at the program start
    keydown, //after keydown
    locked,//after some keydown
};

class Key {
public:
    virtual void event(WPARAM wParam,int vcode) = 0;
};

class ForwardKey:public Key {
public:
    int forwardvcode;
    ForwardKey(int _forwardvcode) :forwardvcode(_forwardvcode) {
    }
    void event(WPARAM wParam, int vcode) {
        send_key(wParam,forwardvcode);
    }
};
class HomeRowKey:public Key {
    KeyState state = init;
    int modevcode;
public:
    HomeRowKey(int _modevcode):modevcode(_modevcode) {
    }
    void event(WPARAM wParam, int vcode) {
        if (state == init) {
            if (main_obj.locked_mods.count(modevcode))
                return send_key(wParam, vcode); //do the default because the mirror homeromod is in effect
            if (is_up(wParam)) {
                printf("unexpexted state for homerow"); //hepfule will next get here
                return send_key(wParam, vcode);
            }
            state = keydown;
            main_obj.locked_mods.insert(modevcode);
            return send_key(wParam, modevcode);
        }
        if (state == keydown) {
            if (is_up(wParam)) {
                send_key(WM_KEYUP, modevcode);
                send_key(WM_KEYDOWN, vcode); //revert the mod
                send_key(WM_KEYUP, vcode);
                main_obj.locked_mods.erase(modevcode);
                state = init;
                return;
            }
            send_key(WM_KEYDOWN, modevcode); //double dowm on the mode as 
            state = locked;
            return;
        }
        if (state == locked) {
            if (is_up(wParam)) {
                send_key(WM_KEYUP, modevcode); //revert the mod
                //dont send the original key because its bein too long
                main_obj.locked_mods.erase(modevcode);
                state = init;
                return;
            }
            send_key(WM_KEYDOWN, modevcode); //double dowm on the mode as 
            return;
        }
    }
};


class LayerKey :public Key {
    Key** layer;
    KeyState state = init;
public:
    LayerKey(Key** _layer) :layer(_layer) {
    }
    void event(WPARAM wParam, int vcode) {
        if (state == init) {
            if (is_up(wParam)) {
                printf("unexpexted state for LayerKey"); //hepfule will not get here
                return send_key(wParam, vcode);
            }
            main_obj.cur_layer = layer;
            state = keydown;
            return;
        }
        if (state == keydown) {
            if (is_up(wParam)) {
                send_key(WM_KEYDOWN, vcode);
                send_key(WM_KEYUP, vcode);
                state = init;
                main_obj.cur_layer = main_obj.top_layer;
                return;
            }
            state = locked;
            return;
        }
        //send_key(wParam, vcode);
        if (is_up(wParam)) {
            main_obj.cur_layer = main_obj.top_layer;
            state = init;
            return;
        }
        //nothing to do on key down

    }

};
Key** make_layer() {
    return new Key * [256] { nullptr };
}
Key** make_nav_layer() {
    const auto ans = make_layer();
    ans['J'] = new ForwardKey(VK_UP);
    ans['M'] = new ForwardKey(VK_DOWN);
    ans['N'] = new ForwardKey(VK_LEFT);
    ans[VK_OEM_COMMA] = new ForwardKey(VK_RIGHT);
    return ans;
}
Key** make_top_layer(){
    const auto ans = make_layer();
    ans['F'] = new HomeRowKey(VK_CONTROL);
    ans['D'] = new HomeRowKey(VK_SHIFT);
    ans['S'] = new HomeRowKey(VK_MENU);
    ans['J'] = new HomeRowKey(VK_CONTROL);
    ans['K'] = new HomeRowKey(VK_SHIFT);
    ans['L'] = new HomeRowKey(VK_MENU);
    ans[' '] = new LayerKey(make_nav_layer());
    return ans;
}
void setup(){
    main_obj.top_layer = make_top_layer();
    main_obj.cur_layer = main_obj.top_layer;
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
    return main_obj.top_layer[vcode];
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
        std::cout << endl<<pcode_to_str(wParam,vcode) << flush;
        if (key == nullptr)
            return false;
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
