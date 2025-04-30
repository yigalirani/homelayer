#include <windows.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <string> 
#include <algorithm> // For std::max
#include <iomanip>
#include <nlohmann/json.hpp>
#include "alg.hpp"

static const char* wmparm_to_tr(WPARAM parm) {
    switch (parm) {
    case WM_KEYDOWN: return "//";
    case WM_KEYUP: return "\\\\";
    case WM_CHAR: return "CHAR";
    case WM_DEADCHAR: return "DCHAR";
    case WM_SYSKEYDOWN: return "SDOWN";
    case WM_SYSKEYUP: return "SUP";
    case WM_SYSCHAR: return "SCHAR";
    case WM_SYSDEADCHAR: return "SDCHAR";
    case WM_UNICHAR: return "UNICHAR";
    default: return "Unknown";
    }
}
static const char* is_down_to_tr(bool is_down) {
    if (is_down)
        return "//";
    return "\\\\";
}
using namespace std;
string to_hexstring(int x) {
    stringstream stream;
    stream << hex << x;
    string result(stream.str());
    return result;
}


static string vcode_to_string(int vcode) {
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
    case VK_OEM_COMMA: return "VK_OEM_COMMA";
    default: return "unknown "+ to_hexstring(vcode);
    }  
}        
/*int maxOfTwo(size_t a, size_t b) {
    return (a > b) ? a : b; // Ternary operator to return the larger value
}*/

string adjustString(const string &input, int length) {
    size_t inputLength = input.length();

    if (inputLength > length) {
        return input.substr(0,length);
    }
    return std::string(input) + std::string(length - inputLength, ' ');
}
static string pcode_to_str(Event e) {
    string ans=string(is_down_to_tr(e.is_down)) + "-" + vcode_to_string(e.vcode); //add padding here so its always the same width
    return ans;
    return adjustString(ans.c_str(), 20);
}
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

std::vector<Event> loadEventsFromFile(const std::string& filename) {
	cout << "Loading events from file: " << filename << endl;
    std::vector<Event> events;

    // Open the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return events;
    }

    // Parse the JSON
    nlohmann::json j;
    try {
        file >> j;
        cout << file.rdbuf();
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return events;
    }

    // Parse each object in the array
    for (const auto& item : j) {
        Event e;
        try {
            e.is_down = item.at("is_down").get<WPARAM>();
            e.vcode = item.at("vcode").get<unsigned char>();
            e.t = item.at("t").get<long long>();
            events.push_back(e);
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing Event: " << e.what() << std::endl;
        }
    }

    return events;
}
