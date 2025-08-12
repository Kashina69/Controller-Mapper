# 🎮 Windows Game Controller to JSON Logger

A lightweight, optimized **C++ application** that reads **old game controller input** and convert them to keyboard and mouse input to PC ( Tested only one windows 10 ) to make them run on newer games which dont support these old cheap duel-shock two type local controllers like i have

* Use a config file **controller_mappings_config.json** to map controller input to keyboard input right joystick is default to mouse btw but you can change it in code

---

## ✨ Features

* 🖱 **Low-latency**: Uses Win32 API for efficient input polling.
* 📄 **JSON Mapping**: Saves controller state to a structured `.json` file.
* ⚡ **Lightweight**: Minimal CPU & RAM usage.
* 🎯 **Cross-game compatibility**: Works with any game controller recognized by Windows.
* 🛠 **Customizable**: Easily change button mapping and polling intervals.

---

## 📂 Project Structure

```
project/
│── main.cpp                # Core application code
│── nlohmann/json.hpp       # JSON library (single header)
│── README.md               # This file
│── controller_mapping_config.json (optional)  # Configuration for mappings and settings ** ( controller to keyboard and mosue ) **
```

---

## ⚙ Requirements

* **Windows OS** (Tested on Windows 10 & 11)
* **MinGW-w64** or any C++ compiler with C++11 or newer
* Game controller recognized by Windows
* [nlohmann/json](https://github.com/nlohmann/json) header file in `nlohmann/json.hpp`

---

## 🚀 Building the Project

### 1️⃣ Clone the Repository 

```bash
git clone https://github.com/Kashina69/Controller Mapper.git
cd Controller Mapper/
```

### 2️⃣ Compile ( Optional ) ( only when you are making change in the code but not in the config )

Using MinGW:

```bash
g++ main.cpp -o mapper -std=c++11 -lwinmm -lshlwapi
```

---

## ▶ Usage ( For non-coding users)

1. **Plug in your controller**.
2. Run the program:

   ```bash
   mapper.exe
   ```
3. By default, the app uses `controller_mapping_config.json` in the same folder to set up your keybinds and settings. If this file is missing, the program will use built-in defaults (which can only be changed by editing and recompiling the code). To customize your controls, simply edit the `controller_mapping_config.json` file—no need to recompile. Make sure both `mapper.exe` and `controller_mapping_config.json` are in the same folder. When you run `mapper.exe`, it will automatically detect and use the config file for mapping your controller. If you want different settings for each game, copy both `mapper.exe` and the config file into each game's folder, edit the config as needed, and run the mapper from there. (Note: Do not rename the config file or its keys—changing these requires modifying and recompiling the code.)

**Example Config file :**

```json
{
  // When you push the left stick up, it will press the W key (move character forward)
  "left_stick_up": "W",
  // When you push the left stick down, it will press the S key (move character backward)
  "left_stick_down": "S",
  // When you push the left stick left, it will press the A key (move character left/strafe left)
  "left_stick_left": "A",
  // When you push the left stick right, it will press the D key (move character right/strafe right)
  "left_stick_right": "D",

  // When you push the right stick up, it will move the mouse up (look up)
  "right_stick_up": "mouse_up",
  // When you push the right stick down, it will move the mouse down (look down)
  "right_stick_down": "mouse_down",
  // When you push the right stick left, it will move the mouse left (look left)
  "right_stick_left": "mouse_left",
  // When you push the right stick right, it will move the mouse right (look right)
  "right_stick_right": "mouse_right",

  // When you press the cross button (X on PlayStation), it will left click the mouse (shoot/jump/confirm)
  "cross": "mouse_left",
  // When you press the circle button (O on PlayStation), it will right click the mouse (aim/cancel)
  "circle": "mouse_right",
  // When you press the square button, it will press the R key (reload/interact)
  "square": "R",
  // When you press the triangle button, it will press the E key (use/interact/switch weapon)
  "triangle": "E",

  // When you press the L1 button, it will press the Shift key (run/sprint)
  "l1": "Shift",
  // When you press the R1 button, it will press the Ctrl key (crouch)
  "r1": "Ctrl",
  // When you press the L2 button, it will left click the mouse (alternate fire/aim)
  "l2": "mouse_left",
  // When you press the R2 button, it will right click the mouse (alternate fire/aim)
  "r2": "mouse_right",
  // When you press the L3 button (press left stick), it will press the P key (custom action)
  "l3": "p",
  // When you press the R3 button (press right stick), it will press the O key (custom action)
  "r3": "o",

  // When you press the D-pad up, it will press the 1 key (select weapon 1)
  "dpad_up": "1",
  // When you press the D-pad down, it will press the 2 key (select weapon 2)
  "dpad_down": "2",
  // When you press the D-pad left, it will press the 3 key (select weapon 3)
  "dpad_left": "3",
  // When you press the D-pad right, it will press the 4 key (select weapon 4)
  "dpad_right": "4",

  // When you press the Start button, it will press the Enter key (pause/menu)
  "start": "Enter",
  // When you press the Select button, it will press the Esc key (back/exit menu)
  "select": "Esc",

  // Deadzone for analog sticks (ignore small stick movements below this value)
  "deadzone": 0.25,
  // Mouse sensitivity multiplier for right stick movement
  "mouse_sensitivity": 14.0,
  // If true, buttons act as "hold" (send key down while held), otherwise as "tap"
  "hold_mode_buttons": true
}

```

---

## ⚡ Performance Notes

* Polling rate is controlled by `poll_ms` in the code (default: 16ms for \~60FPS input rate).
* JSON file writes are batched to reduce disk I/O overhead.
* Uses `nlohmann/json` for fast serialization.

---

## 🛠 Customization

* **Mapping buttons**: Change the button names in the code to match your needs.
* **Adding more inputs**: Modify `main.cpp` to read triggers, D-pad, etc.
* **Output path**: Update the output file location in the source code.

---

## 📜 License

MIT License.
You can use, modify, and distribute this freely as long as you keep this notice.

---

## 🙌 Credits

* **nlohmann/json** for JSON parsing/serialization.
* Windows API for input handling.

---