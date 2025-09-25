#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>  
#include <set> 
#include "utils.hpp"
#include "alg.hpp"
#include <cassert>
#include <bitset>   
//#define HOMELAYER_MAGIC 0xDEADBEEF
bool is_extended(int vcode) {
    return vcode == VK_INSERT ||
        vcode == VK_DELETE ||
        vcode == VK_HOME ||
        vcode == VK_END ||
        vcode == VK_PRIOR || // Page Up
        vcode == VK_NEXT || // Page Down
        vcode == VK_LEFT ||
        vcode == VK_RIGHT ||
        vcode == VK_UP ||
        vcode == VK_DOWN;
}
string highlight(string txt, const char* color) {
    return string("\033[") + color + "m " + txt + "\033[0m";
}

void send_key(vector<Event> events,bool fake) {
	vector<INPUT> inputs;
	for (auto e : events) {
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = e.vcode;
		input.ki.wScan = 0;
        input.ki.dwFlags = (e.is_down ? 0 : KEYEVENTF_KEYUP) | is_extended(e.vcode) & KEYEVENTF_EXTENDEDKEY;
		input.ki.time = 0;
		//input.ki.dwExtraInfo = HOMELAYER_MAGIC;
        //if (fake)
            cout << highlight(pcode_to_str(e),"32")<< endl;
		inputs.push_back(input);
		
	}
    if (fake)
        return;
	SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
}
class MainObject {
public:
    HHOOK hHook = 0;
    long long recording_start_time = get_cur_time();
    vector<Event> recorded_events;
    //Alg* alg = make_delay_alg();// make_doubling_alg();
    //Alg* alg = make_pass_through();
    //Alg* alg = make_doubling_alg();
    //Alg* alg = make_delay_alg();
    Alg* alg = make_alt_alg();

}main_obj;

boolean escape_mode = false;
void handle_escape_mode(Event& e){
    if (!e.is_down)
        return;
    if (escape_mode && e.vcode == 123) {//f_12
        cout << highlight("saving", "33") << "\n" << flush;
        write_events(main_obj.recorded_events);
        main_obj.recorded_events.clear();

    }
    if (escape_mode && e.vcode == 121) {//f_11
        cout << "exit" << flush;
        exit(0);
    }
    auto last_escape_mode = escape_mode;
    escape_mode = (e.vcode == VK_ESCAPE);
    if (escape_mode)
        cout << highlight("escape mode on", "33");
    if (last_escape_mode && !escape_mode)
        cout << highlight("escape mode off", "33");
}

void handle_event(Event& e,bool fake) {
    main_obj.recorded_events.push_back(e);
    auto gen_events=main_obj.alg->handle_event(e);
    for (Event e2 : gen_events) {
        e2.comment = "out_";
        e2.t = e.t;//because we are sending it right now
        handle_escape_mode(e);
        main_obj.recorded_events.push_back(e2);
    }
    send_key(gen_events,fake);
    //handle_event_delayed_down(e);
    //handle_event_pass_through(e);
}



LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    KBDLLHOOKSTRUCT* pKbDllHookStruct = (KBDLLHOOKSTRUCT*)lParam;
    auto doit = [&]() {  
        if (nCode != HC_ACTION)             
            return false;
        if (pKbDllHookStruct->flags&16)//dwExtraInfo == HOMELAYER_MAGIC)
            return false; //skipfget-curzped to avoid proccesing      our own synthetic 
            


        unsigned char vcode = (unsigned char)pKbDllHookStruct->vkCode;
        cout << "VK: " << vcode_to_string(vcode) << ", wParam: " << wParam << ", flags: " << pKbDllHookStruct->flags<< " "<<bitset<8 >(pKbDllHookStruct->flags)  
            << ", extended: " << ((pKbDllHookStruct->flags & LLKHF_EXTENDED) ? "yes" : "no") << endl;
        auto is_down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
        auto  t = get_cur_time() - main_obj.recording_start_time;
		Event event = { vcode, is_down, t };


		if (main_obj.recorded_events.size() == 0 && !event.is_down){
            cout << "skipping first" << endl;
        }
        else {
            
            handle_event(event,false);

        }


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
void run(const string& filename) {
}
/*void wait_for_it(long long t, long long start) {
	int sleep_count = 0;
    while (true) {
        auto cur = ge t_cur_time() - start;
        auto diff = cur-t;
        if (diff > 0) {
            if (sleep_count)
                cout << "sleep_count:" << sleep_count << endl;
            return;
        }
        Sleep(1);
        sleep_count++;
    }
    
}*/
void play(const string& filename) {
	cout << "playing" << filename<< endl;
    //auto start = ge t_cur_time();
	auto events = loadEventsFromFile(filename);
	for (auto event : events) {
        //wait_for_it(event.t, start);
		handle_event(event,true);
	}
}
int main(int argc, char* argv[]) {
    //setup_layers();
    if (argc >= 2) {
        string command = argv[1];
        string file_name = argc == 3 ? argv[1] : "output.json";

        if (command == "run") {
            run(file_name);
            return 0;
        }
        if (command == "play") {
            play(file_name);
            return 0;
        }
        cerr << "Unknown command: " << command << endl;
        return 1;
    }
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
