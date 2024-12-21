#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>  
#include <set> 
#include "utils.hpp"
void send_key(WPARAM wParam, int vcode) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vcode;
    input.ki.wScan = 0;
    input.ki.dwFlags = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ? KEYEVENTF_KEYUP : 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}
boolean is_up(WPARAM wParam) {
    return wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
}
boolean is_down(WPARAM wParam) {
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
                send_key(wParam, modevcode); //revert the mod
                send_key(WM_KEYDOWN, vcode); //send the original keym both down and up
                send_key(WM_KEYUP, vcode);
                main_obj.locked_mods.erase(modevcode);
                state = init;
                return;
            }
            send_key(wParam, modevcode); //double dowm on the mode as 
            state = locked;
            return;
        }
        if (state == locked) {
            if (is_up(wParam)) {
                send_key(wParam, modevcode); //revert the mod
                //dont send the original key because its bein too long
                main_obj.locked_mods.erase(modevcode);
                state = init;
                return;
            }
            send_key(wParam, modevcode); //double dowm on the mode as 
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
        send_key(wParam, vcode);
        if (is_down(wParam)) {
        }
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
    ans[','] = new ForwardKey(VK_LEFT);
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
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKbDllHookStruct = (KBDLLHOOKSTRUCT*)lParam;
        std::cout << wmparm_to_tr(wParam) << ':' << vcode_to_string(pKbDllHookStruct->vkCode) << std::endl;
        auto vcode = pKbDllHookStruct->vkCode;
        auto key = main_obj.cur_layer[vcode];
        if (key== nullptr)
            return CallNextHookEx(main_obj.hHook, nCode, wParam, lParam);
        key->event(wParam, vcode);
        return 1; //suppress original
    }

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
