#include <windows.h>
#include <stdio.h>
#include <iostream>

// Global hook handle
HHOOK hHook;
static const char* wmparm_to_tr(int parm) {
    switch (parm) {
    case 0x0100: return "WM_KEYDOWN";
    case 0x0101: return "WM_KEYUP";
    case 0x0102: return "WM_CHAR";
    case 0x0103: return "WM_DEADCHAR";
    case 0x0104: return "WM_SYSKEYDOWN";
    case 0x0105: return "WM_SYSKEYUP";
    case 0x0106: return "WM_SYSCHAR";
    case 0x0107: return "WM_SYSDEADCHAR";
    case 0x0109: return "WM_UNICHAR";
    default: return "Unknown WM code";
    }
}
#include <windows.h>

static const char* vcode_to_string(int vcode) {
    static char buffer[2]; // For single-character keys (e.g., 'A', '1')

    // Alphanumeric keys
    if (vcode >= 'A' && vcode <= 'Z') {
        buffer[0] = (char)vcode; // Convert vkCode to character
        buffer[1] = '\0';        // Null-terminate the string
        return buffer;
    }
    if (vcode >= '0' && vcode <= '9') {
        buffer[0] = (char)vcode; // Convert vkCode to character
        buffer[1] = '\0';        // Null-terminate the string
        return buffer;
    }

    // Special keys
    switch (vcode) {
    case VK_BACK: return "Backspace";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Control";
    case VK_MENU: return "Alt";
    case VK_PAUSE: return "Pause";
    case VK_CAPITAL: return "Caps Lock";
    case VK_ESCAPE: return "Escape";
    case VK_SPACE: return "Space";
    case VK_PRIOR: return "Page Up";
    case VK_NEXT: return "Page Down";
    case VK_END: return "End";
    case VK_HOME: return "Home";
    case VK_LEFT: return "Left Arrow";
    case VK_UP: return "Up Arrow";
    case VK_RIGHT: return "Right Arrow";
    case VK_DOWN: return "Down Arrow";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_LWIN: return "Left Windows Key";
    case VK_RWIN: return "Right Windows Key";
    case VK_APPS: return "Applications Key";
    case VK_NUMPAD0: return "Numpad 0";
    case VK_NUMPAD1: return "Numpad 1";
    case VK_NUMPAD2: return "Numpad 2";
    case VK_NUMPAD3: return "Numpad 3";
    case VK_NUMPAD4: return "Numpad 4";
    case VK_NUMPAD5: return "Numpad 5";
    case VK_NUMPAD6: return "Numpad 6";
    case VK_NUMPAD7: return "Numpad 7";
    case VK_NUMPAD8: return "Numpad 8";
    case VK_NUMPAD9: return "Numpad 9";
    case VK_MULTIPLY: return "Numpad Multiply";
    case VK_ADD: return "Numpad Add";
    case VK_SEPARATOR: return "Numpad Separator";
    case VK_SUBTRACT: return "Numpad Subtract";
    case VK_DECIMAL: return "Numpad Decimal";
    case VK_DIVIDE: return "Numpad Divide";
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    case VK_NUMLOCK: return "Num Lock";
    case VK_SCROLL: return "Scroll Lock";
    case VK_LSHIFT: return "VK_LSHIFT";
    case VK_RSHIFT: return "VK_RSHIFT";
    case VK_LCONTROL: return "VK_LCONTROL";
    case VK_RCONTROL: return "VK_RCONTROL";
    case VK_LMENU: return "VK_LMENU";
    case VK_RMENU: return "VK_RMENU";
    default: return "Unknown Key";
    }
}
enum Lockable {
    shift = 0,
    alt,
    ctrl
};
bool locables[ctrl + 1] = { false };
#define EPOCH_DIFFERENCE 11644473600000LL // Difference between 1601 and 1970 in milliseconds

long long get_cur_time() {
    FILETIME fileTime;
    ULARGE_INTEGER largeInt;

    // Get the current system time as FILETIME
    GetSystemTimeAsFileTime(&fileTime);

    // Combine the two 32-bit values into a 64-bit value
    largeInt.LowPart = fileTime.dwLowDateTime;
    largeInt.HighPart = fileTime.dwHighDateTime;

    // Convert to milliseconds (from 100-nanosecond intervals) and subtract the epoch difference
    return (largeInt.QuadPart / 10000) - EPOCH_DIFFERENCE;
}
enum KeyState {
    init = 0, //at the program start
    keydown, //after keydown
    locked,//after some keydown
};

void send_key(int dwFlags, int vcode) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vcode;
    input.ki.wScan = 0;
    input.ki.dwFlags = dwFlags;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));

}
class Key {
public:
    virtual void event(int wParam,int vcode) = 0;
};
static int wparam_to_eventf(int wParam) {
    return (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ? KEYEVENTF_KEYUP : 0;

}
class ForwardKey:public Key {
public:
    int forwardvcode;
    ForwardKey(int _forwardvcode) :forwardvcode(_forwardvcode) {
    }
    void event(int wParam, int vcode) {
        send_key(wparam_to_eventf(wParam),forwardvcode);
    }
};
class HomeRowKey:public Key {
    KeyState state = init;
    Lockable lockable;
public:
    HomeRowKey(Lockable l) :lockable(l) {
    }
    void event(int wParam, int vcode) {
        send_key(wparam_to_eventf(wParam), vcode);
    }
};
class LayerKey :public Key {
    const Key** layer;
public:
    LayerKey(Key** _layer) :layer(_layer) {
    }
    void event(int wParam, int vcode) {
        send_key(wparam_to_eventf(wParam), vcode);
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
Key** make_main_layer(){
    const auto ans = make_layer();
    ans['F'] = new HomeRowKey(ctrl);
    ans['D'] = new HomeRowKey(shift);
    ans['S'] = new HomeRowKey(alt);
    ans['J'] = new HomeRowKey(ctrl);
    ans['K'] = new HomeRowKey(shift);
    ans['L'] = new HomeRowKey(alt);
    ans[' '] = new LayerKey(make_nav_layer());
    return ans;
}
Key** const main_layer = make_main_layer();
Key ** cur_layer = main_layer;
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
        auto key = cur_layer[vcode];
        if (key== nullptr)
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        key->event(wParam, vcode);
        return 1; //suppress original
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// Function to install the hook
void InstallHook() {
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (hHook == NULL) {
        printf("Failed to install hook! Error: %lu\n", GetLastError());
        exit(EXIT_FAILURE);
    }
    else {
        printf("Hook installed successfully!\n");
    }
}

// Function to uninstall the hook
void UninstallHook() {
    if (hHook != NULL) {
        UnhookWindowsHookEx(hHook);
        printf("Hook uninstalled successfully!\n");
    }
}

int main() {
    MSG msg;

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
