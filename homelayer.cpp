#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>  
#include <set> 
#include "utils.hpp"
#include "alg.hpp"
#include <cassert>
#define HOMELAYER_MAGIC 0xDEADBEEF

void send_key(vector<Event> events) {
	vector<INPUT> inputs;
	for (auto e : events) {
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = e.vcode;
		input.ki.wScan = 0;
		input.ki.dwFlags = e.is_down?0:KEYEVENTF_KEYUP;
		input.ki.time = 0;
		input.ki.dwExtraInfo = HOMELAYER_MAGIC;
		inputs.push_back(input);
		
	}
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
    Alg* alg = make_delayed_down();
    

}main_obj;


void handle_event(Event& e) {
    main_obj.recorded_events.push_back(e);
    auto gen_events=main_obj.alg->handle_event(e);
    for (Event e2 : gen_events) {
        e2.comment = "out_";
        e2.t = e.t;//because we are sending it right now
        main_obj.recorded_events.push_back(e2);
    }
    send_key(gen_events);
    cout << "\033[32m " << pcode_to_str(e) << "\033[0m" << endl;
    //handle_event_delayed_down(e);
    //handle_event_pass_through(e);
}



LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    KBDLLHOOKSTRUCT* pKbDllHookStruct = (KBDLLHOOKSTRUCT*)lParam;
    auto doit = [&]() {  
        if (nCode != HC_ACTION)             
            return false;
        if (pKbDllHookStruct->dwExtraInfo == HOMELAYER_MAGIC)
            return false; //skipfget-curzped to avoid proccesing      our own synthetic 
        unsigned char vcode = (unsigned char)pKbDllHookStruct->vkCode;
        auto is_down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
        auto  t = get_cur_time() - main_obj.recording_start_time;
		Event event = { vcode, is_down, t };
        if (vcode == 123 && is_down) {//f_12
            cout << "saving\n" << flush;
            write_events(main_obj.recorded_events);
            main_obj.recorded_events.clear();
            return true;
        }
        if (vcode == 121) {//f_11
			cout << "exit" << flush;    
			exit(0);
        }
		if (main_obj.recorded_events.size() == 0 && !event.is_down){
            cout << "skipping first" << endl;
        }
        else {
            
            handle_event(event);

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
void wait_for_it(long long t, long long start) {
	int sleep_count = 0;
    while (true) {
        auto cur = get_cur_time() - start;
        auto diff = cur-t;
        if (diff > 0) {
            if (sleep_count)
                cout << "sleep_count:" << sleep_count << endl;
            return;
        }
        Sleep(1);
        sleep_count++;
    }
    
}
void play(const string& filename) {
	cout << "playing" << filename<< endl;
    auto start = get_cur_time();
	auto events = loadEventsFromFile(filename);
	for (auto event : events) {
        wait_for_it(event.t, start);
		handle_event(event);
	}
}
int main(int argc, char* argv[]) {
    //setup_layers();
    if (argc == 3) {
        std::string command = argv[1];
        std::string file_name = argv[2];

        if (command == "run") {
            run(file_name);
            return 0;
        }
        if (command == "play") {
            play(file_name);
            return 0;
        }
        std::cerr << "Unknown command: " << command << std::endl;
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
