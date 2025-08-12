// main.cpp
// Compile: g++ -std=c++17 main.cpp -o mapper.exe -lshlwapi -lwinmm
// or MSVC: cl /EHsc main.cpp user32.lib winmm.lib shlwapi.lib

#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shlwapi.h>   // PathRemoveFileSpecA
#include <mmsystem.h>  // joyGetPosEx
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// ---------- Utilities ----------
static std::string toLower(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return std::tolower(c); });
    return out;
}

std::string getExeDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    return std::string(path);
}

void logInfo(const std::string &s){ std::cout << "[INFO] " << s << "\n"; }
void logWarn(const std::string &s){ std::cerr << "[WARN] " << s << "\n"; }
void logErr(const std::string &s){ std::cerr << "[ERROR] " << s << "\n"; }

// ---------- SendInput helpers ----------
void sendKeyVK(WORD vk, bool press) {
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = press ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(in));
}

void sendMouseMove(int dx, int dy) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    in.mi.dx = dx;
    in.mi.dy = dy;
    SendInput(1, &in, sizeof(in));
}

void sendMouseClickLeft(bool down) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    SendInput(1, &in, sizeof(in));
}
void sendMouseClickRight(bool down) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    SendInput(1, &in, sizeof(in));
}

// ---------- Key mapping ----------
WORD vkFromString(const std::string &raw) {
    if (raw.empty()) return 0;
    std::string s = toLower(raw);

    // mouse special names handled elsewhere (mouse_left, mouse_right, mouse_up, etc.)
    static std::unordered_map<std::string, WORD> map = {
        // letters
        {"a",'A'}, {"b",'B'}, {"c",'C'}, {"d",'D'}, {"e",'E'}, {"f",'F'}, {"g",'G'}, {"h",'H'},
        {"i",'I'}, {"j",'J'}, {"k",'K'}, {"l",'L'}, {"m",'M'}, {"n",'N'}, {"o",'O'}, {"p",'P'},
        {"q",'Q'}, {"r",'R'}, {"s",'S'}, {"t",'T'}, {"u",'U'}, {"v",'V'}, {"w",'W'}, {"x",'X'},
        {"y",'Y'}, {"z",'Z'},
        // numbers
        {"0",'0'}, {"1",'1'}, {"2",'2'}, {"3",'3'}, {"4",'4'}, {"5",'5'}, {"6",'6'}, {"7",'7'}, {"8",'8'}, {"9",'9'},
        // words
        {"space", VK_SPACE}, {"enter", VK_RETURN}, {"esc", VK_ESCAPE}, {"escape", VK_ESCAPE},
        {"tab", VK_TAB}, {"shift", VK_SHIFT}, {"ctrl", VK_CONTROL}, {"control", VK_CONTROL},
        {"alt", VK_MENU}, {"capslock", VK_CAPITAL}, {"backspace", VK_BACK}, {"ins", VK_INSERT}, {"del", VK_DELETE},
        {"home", VK_HOME}, {"end", VK_END}, {"pgup", VK_PRIOR}, {"pgdn", VK_NEXT},
        {"left", VK_LEFT}, {"right", VK_RIGHT}, {"up", VK_UP}, {"down", VK_DOWN},
        {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4}, {"f5", VK_F5},
        {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8}, {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12}
    };

    auto it = map.find(s);
    if (it != map.end()) return it->second;
    return 0;
}

// ---------- Config struct ----------
struct Config {
    // mapping for raw button index -> action string (action may be "mouse_left" or a key string)
    std::unordered_map<int, std::string> buttonAction;

    // logical mappings for sticks & dpad (action strings)
    std::string left_up="w", left_down="s", left_left="a", left_right="d";
    std::string right_up="mouse_up", right_down="mouse_down", right_left="mouse_left", right_right="mouse_right";

    std::string cross="mouse_left", circle="mouse_right", square="r", triangle="e";
    std::string l1="shift", r1="ctrl", l2="mouse_left", r2="mouse_right";
    std::string start="enter", select="esc";
    std::string l3="c", r3="v";

    // dpad can be nested or flat
    std::string dpad_up="", dpad_down="", dpad_left="", dpad_right="";

    float deadzone = 0.25f; // 0..1
    float mouse_sensitivity = 14.0f;
    bool hold_mode_buttons = true;
};

// default semantic -> button index (common)
const std::map<std::string,int> defaultButtonIndex = {
    {"cross", 0}, {"circle", 1}, {"square", 2}, {"triangle", 3},
    {"l1", 4}, {"r1", 5}, {"l2", 6}, {"r2", 7},
    {"select", 8}, {"start", 9}, {"l3", 10}, {"r3", 11}
};

// parse config JSON into our Config
Config loadConfig() {
    Config cfg;
    std::string path = getExeDir() + "\\controller_mappings_config.json";
    std::ifstream in(path);
    if (!in) {
        logWarn("Config not found: " + path + ". Using defaults.");
        return cfg;
    }

    try {
        json j; in >> j;

        // simple flat keys
        if (j.contains("left_stick_up")) cfg.left_up = j["left_stick_up"];
        if (j.contains("left_stick_down")) cfg.left_down = j["left_stick_down"];
        if (j.contains("left_stick_left")) cfg.left_left = j["left_stick_left"];
        if (j.contains("left_stick_right")) cfg.left_right = j["left_stick_right"];

        if (j.contains("right_stick_up")) cfg.right_up = j["right_stick_up"];
        if (j.contains("right_stick_down")) cfg.right_down = j["right_stick_down"];
        if (j.contains("right_stick_left")) cfg.right_left = j["right_stick_left"];
        if (j.contains("right_stick_right")) cfg.right_right = j["right_stick_right"];

        if (j.contains("cross")) cfg.cross = j["cross"];
        if (j.contains("circle")) cfg.circle = j["circle"];
        if (j.contains("square")) cfg.square = j["square"];
        if (j.contains("triangle")) cfg.triangle = j["triangle"];

        if (j.contains("l1")) cfg.l1 = j["l1"];
        if (j.contains("r1")) cfg.r1 = j["r1"];
        if (j.contains("l2")) cfg.l2 = j["l2"];
        if (j.contains("r2")) cfg.r2 = j["r2"];

        if (j.contains("l3")) cfg.l3 = j["l3"];
        if (j.contains("r3")) cfg.r3 = j["r3"];

        if (j.contains("start")) cfg.start = j["start"];
        if (j.contains("select")) cfg.select = j["select"];

        // nested dpad or flat keys
        if (j.contains("dpad") && j["dpad"].is_object()) {
            auto &d = j["dpad"];
            if (d.contains("up")) cfg.dpad_up = d["up"];
            if (d.contains("down")) cfg.dpad_down = d["down"];
            if (d.contains("left")) cfg.dpad_left = d["left"];
            if (d.contains("right")) cfg.dpad_right = d["right"];
        }
        if (j.contains("dpad_up")) cfg.dpad_up = j["dpad_up"];
        if (j.contains("dpad_down")) cfg.dpad_down = j["dpad_down"];
        if (j.contains("dpad_left")) cfg.dpad_left = j["dpad_left"];
        if (j.contains("dpad_right")) cfg.dpad_right = j["dpad_right"];

        // legacy numeric "buttons" mapping (0..n) -> action string
        if (j.contains("buttons") && j["buttons"].is_object()) {
            for (auto &el : j["buttons"].items()) {
                try {
                    int idx = std::stoi(el.key());
                    cfg.buttonAction[idx] = el.value().get<std::string>();
                } catch(...) { logWarn("Invalid numeric button key in config: " + el.key()); }
            }
        }

        // If flat semantic names present, map them to indices
        for (auto &p : defaultButtonIndex) {
            const std::string &name = p.first;
            int idx = p.second;
            if (j.contains(name)) {
                cfg.buttonAction[idx] = j[name].get<std::string>();
            }
        }

        // extras
        if (j.contains("deadzone")) cfg.deadzone = j["deadzone"];
        if (j.contains("mouse_sensitivity")) cfg.mouse_sensitivity = j["mouse_sensitivity"];
        if (j.contains("hold_mode_buttons")) cfg.hold_mode_buttons = j["hold_mode_buttons"];

    } catch (std::exception &e) {
        logErr(std::string("Failed parse config: ") + e.what() + " — using defaults.");
    }

    return cfg;
}

// ---------- Main mapping runtime ----------

bool isMouseAction(const std::string &act) {
    std::string s = toLower(act);
    return s.find("mouse") == 0; // mouse_left, mouse_right, mouse_up/down/left/right
}

void performButtonActionPress(const std::string &action, std::unordered_map<std::string,bool> &heldKeys, std::unordered_map<std::string,bool> &heldMouse) {
    std::string a = toLower(action);
    if (a == "") return;
    if (a == "mouse_left") { sendMouseClickLeft(true); heldMouse["left"] = true; return; }
    if (a == "mouse_right") { sendMouseClickRight(true); heldMouse["right"] = true; return; }
    // key
    WORD vk = vkFromString(a);
    if (vk) {
        if (!heldKeys[a]) {
            sendKeyVK(vk, true);
            heldKeys[a] = true;
        }
    } else {
        // unknown: ignore
    }
}

void performButtonActionRelease(const std::string &action, std::unordered_map<std::string,bool> &heldKeys, std::unordered_map<std::string,bool> &heldMouse) {
    std::string a = toLower(action);
    if (a == "") return;
    if (a == "mouse_left") { if (heldMouse["left"]) { sendMouseClickLeft(false); heldMouse["left"] = false; } return; }
    if (a == "mouse_right") { if (heldMouse["right"]) { sendMouseClickRight(false); heldMouse["right"] = false; } return; }
    WORD vk = vkFromString(a);
    if (vk) {
        if (heldKeys[a]) {
            sendKeyVK(vk, false);
            heldKeys[a] = false;
        }
    }
}

// For axis-based mapping to keys (left stick digital)
void handleAxisKey(const std::string &action, bool on, std::unordered_map<std::string,bool> &heldKeys) {
    if (action.empty()) return;
    std::string a = toLower(action);
    WORD vk = vkFromString(a);
    if (!vk) return;
    if (on && !heldKeys[a]) { sendKeyVK(vk, true); heldKeys[a] = true; }
    if (!on && heldKeys[a]) { sendKeyVK(vk, false); heldKeys[a] = false; }
}

int main(int argc, char** argv) {
    logInfo("Controller mapper starting...");
    Config cfg = loadConfig();

    // Show loaded mapping summary (basic)
    logInfo("Deadzone: " + std::to_string(cfg.deadzone) + " mouse_sens: " + std::to_string(cfg.mouse_sensitivity));

    // Held state
    std::unordered_map<int,bool> prevBtnState;
    std::unordered_map<std::string,bool> heldKeys;   // action string -> currently held
    std::unordered_map<std::string,bool> heldMouse;  // left/right

    const int poll_ms = 8; // ~125Hz

    // Main loop
    while (true) {
        // read joystick 0
        JOYINFOEX ji;
        ji.dwSize = sizeof(JOYINFOEX);
        ji.dwFlags = JOY_RETURNALL;
        MMRESULT res = joyGetPosEx(0, &ji);
        if (res == JOYERR_NOERROR) {
            DWORD buttons = ji.dwButtons; // bitmask for buttons 0..31

            // handle numeric button mapping (from cfg.buttonAction)
            for (auto &pair : cfg.buttonAction) {
                int bidx = pair.first;
                std::string action = pair.second;

                bool pressed = (buttons & (1u << bidx)) != 0;
                bool prev = prevBtnState[bidx];

                if (pressed && !prev) {
                    // pressed edge
                    performButtonActionPress(action, heldKeys, heldMouse);
                }
                if (!pressed && prev) {
                    // release edge
                    performButtonActionRelease(action, heldKeys, heldMouse);
                }
                prevBtnState[bidx] = pressed;
            }

            // handle semantic buttons via default mapping if not provided numerically
            for (auto &p : defaultButtonIndex) {
                int idx = p.second;
                if (cfg.buttonAction.find(idx) != cfg.buttonAction.end()) continue; // already handled

                bool pressed = (buttons & (1u << idx)) != 0;
                bool prev = prevBtnState[idx];

                // pick action from cfg fields
                std::string action;
                if (p.first == "cross") action = cfg.cross;
                else if (p.first == "circle") action = cfg.circle;
                else if (p.first == "square") action = cfg.square;
                else if (p.first == "triangle") action = cfg.triangle;
                else if (p.first == "l1") action = cfg.l1;
                else if (p.first == "r1") action = cfg.r1;
                else if (p.first == "l2") action = cfg.l2;
                else if (p.first == "r2") action = cfg.r2;
                else if (p.first == "select") action = cfg.select;
                else if (p.first == "start") action = cfg.start;
                else if (p.first == "l3") action = cfg.l3;
                else if (p.first == "r3") action = cfg.r3;

                if (action.empty()) { prevBtnState[idx] = pressed; continue; }

                if (pressed && !prev) performButtonActionPress(action, heldKeys, heldMouse);
                if (!pressed && prev) performButtonActionRelease(action, heldKeys, heldMouse);
                prevBtnState[idx] = pressed;
            }

            // Left stick axes -> key presses (digital)
            float lx = ((float)ji.dwXpos - 32767.5f) / 32767.5f;
            float ly = ((float)ji.dwYpos - 32767.5f) / 32767.5f;
            bool up = (ly < -cfg.deadzone);
            bool down = (ly > cfg.deadzone);
            bool left = (lx < -cfg.deadzone);
            bool right = (lx > cfg.deadzone);

            handleAxisKey(cfg.left_up, up, heldKeys);
            handleAxisKey(cfg.left_down, down, heldKeys);
            handleAxisKey(cfg.left_left, left, heldKeys);
            handleAxisKey(cfg.left_right, right, heldKeys);

            // Right stick -> mouse movement OR key mapping
            float rx = ((float)ji.dwZpos - 32767.5f) / 32767.5f;
            float ry = ((float)ji.dwRpos - 32767.5f) / 32767.5f;
            // small deadzone
            if (std::fabs(rx) < 0.08f) rx = 0.0f;
            if (std::fabs(ry) < 0.08f) ry = 0.0f;

            // If right_* map to mouse directions, move mouse.
            // We'll check if any of them equal "mouse_left" etc.
            bool rightUsesMouse = (toLower(cfg.right_left).find("mouse") == 0 ||
                                   toLower(cfg.right_right).find("mouse") == 0 ||
                                   toLower(cfg.right_up).find("mouse") == 0 ||
                                   toLower(cfg.right_down).find("mouse") == 0);

            if (rightUsesMouse) {
                if (rx != 0.0f || ry != 0.0f) {
                    int dx = (int)(rx * cfg.mouse_sensitivity);
                    int dy = (int)(-ry * cfg.mouse_sensitivity);
                    if (dx != 0 || dy != 0) sendMouseMove(dx, dy);
                }
            } else {
                // treat RHS as keys (digital by direction)
                handleAxisKey(cfg.right_up, (ry < -cfg.deadzone), heldKeys);
                handleAxisKey(cfg.right_down, (ry > cfg.deadzone), heldKeys);
                handleAxisKey(cfg.right_left, (rx < -cfg.deadzone), heldKeys);
                handleAxisKey(cfg.right_right, (rx > cfg.deadzone), heldKeys);
            }

            // D-Pad (POV) handling - dwPOV in hundredths of a degree; 0=up, 9000=right, -1=centered
            DWORD pov = ji.dwPOV; // 0..35999 or 0xFFFFFFFF if not pressed
            // clear previous dpad states when not pressed
            static bool prevDpadUp=false, prevDpadDown=false, prevDpadLeft=false, prevDpadRight=false;
            bool curUp=false, curDown=false, curLeft=false, curRight=false;
            if (pov != JOY_POVCENTERED) {
                // Map ranges (including diagonals)
                // up: angle [31500..36000) or [0..4500]
                if ( (pov >= 31500 && pov <= 35999) || (pov >=0 && pov <=4500) ) curUp = true;
                if (pov >= 4500 && pov <= 13500) curRight = true;
                if (pov >= 13500 && pov <= 22500) curDown = true;
                if (pov >= 22500 && pov <= 31500) curLeft = true;
            }

            // handle transitions for each dpad direction using cfg.dpad_* if present
            auto handleDpadEdge = [&](const std::string &act, bool cur, bool &prev){
                if (act.empty()) { prev = cur; return; }
                if (cur && !prev) performButtonActionPress(act, heldKeys, heldMouse);
                if (!cur && prev) performButtonActionRelease(act, heldKeys, heldMouse);
                prev = cur;
            };

            handleDpadEdge(cfg.dpad_up, curUp, prevDpadUp);
            handleDpadEdge(cfg.dpad_down, curDown, prevDpadDown);
            handleDpadEdge(cfg.dpad_left, curLeft, prevDpadLeft);
            handleDpadEdge(cfg.dpad_right, curRight, prevDpadRight);
        } // else ignore read errors silently

        Sleep(poll_ms);
    }

    // On exit: release all held actions
    for (auto &p : heldKeys) {
        if (p.second) {
            WORD vk = vkFromString(p.first);
            if (vk) sendKeyVK(vk, false);
        }
    }
    if (heldMouse["left"]) sendMouseClickLeft(false);
    if (heldMouse["right"]) sendMouseClickRight(false);

    return 0;
}

// g++ -std=c++17 main.cpp -o mapper.exe -lshlwapi -lwinmm
