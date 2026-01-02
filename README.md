# 6-Motor Parallel Controller with EEPROM State Persistence

A robust, non-blocking stepper motor controller for ESP32 that manages 6 motors simultaneously with automatic state restoration after power interruption.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![Language](https://img.shields.io/badge/language-C++-orange.svg)

## 📋 Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Installation](#installation)
- [How It Works](#how-it-works)
- [Usage](#usage)
- [Serial Commands](#serial-commands)
- [Swing Modes](#swing-modes)
- [EEPROM Memory Layout](#eeprom-memory-layout)
- [Power Recovery Sequence](#power-recovery-sequence)
- [Technical Specifications](#technical-specifications)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## ✨ Features

### Core Functionality
- **6 Independent Stepper Motors** - Non-blocking parallel control
- **State Persistence** - EEPROM storage survives power loss
- **Automatic Recovery** - Motors restore previous positions after power interruption
- **IR Sensor Homing** - Precise positioning with sensor-based calibration
- **Swing Modes** - 4 preset oscillation patterns for automated movement
- **Serial Control** - UART command interface for real-time control

### Smart Recovery System
- ✅ On power-up, motors auto-scan to find home position
- ✅ After homing, automatically restore saved positions
- ✅ Resume swing operations if active before power loss
- ✅ Independent recovery for each motor

### Advanced Features
- Non-blocking operation (all motors move simultaneously)
- Real-time position tracking
- Serial logging for debugging
- Configurable step resolution (default: 1600 steps)
- Individual and group motor control

---

## 🛠 Hardware Requirements

### Components
- **Microcontroller:** ESP32 Development Board
- **Stepper Motors:** 6x NEMA stepper motors (or compatible)
- **Motor Drivers:** 6x Stepper motor drivers (A4988, DRV8825, or similar)
- **Sensors:** 6x IR proximity sensors (for homing)
- **Power Supply:** Appropriate voltage/current for your motors

## 📌 Pin Configuration

| Motor | STEP Pin | DIR Pin | IR Sensor Pin |
|-------|----------|---------|---------------|
| 1     | 13       | 12      | 14            |
| 2     | 27       | 26      | 25            |
| 3     | 21       | 22      | 32            |
| 4     | 15       | 2       | 0             |
| 5     | 4        | 16      | 17            |
| 6     | 5        | 18      | 19            |

> **Note:** Pins can be modified in the code's PIN CONFIG section.

---

## 💾 Installation

### 1. Arduino IDE Setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to `File > Preferences`
   - Add to Additional Board Manager URLs:
   - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   - - Go to `Tools > Board > Boards Manager`
   - Search for "ESP32" and install
# 6-Motor Parallel Controller with EEPROM State Persistence

A robust, non-blocking stepper motor controller for ESP32 that manages 6 motors simultaneously with automatic state restoration after power interruption.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![Language](https://img.shields.io/badge/language-C++-orange.svg)

## 📋 Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Installation](#installation)
- [How It Works](#how-it-works)
- [Usage](#usage)
- [Serial Commands](#serial-commands)
- [Swing Modes](#swing-modes)
- [EEPROM Memory Layout](#eeprom-memory-layout)
- [Power Recovery Sequence](#power-recovery-sequence)
- [Technical Specifications](#technical-specifications)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## ✨ Features

### Core Functionality
- **6 Independent Stepper Motors** - Non-blocking parallel control
- **State Persistence** - EEPROM storage survives power loss
- **Automatic Recovery** - Motors restore previous positions after power interruption
- **IR Sensor Homing** - Precise positioning with sensor-based calibration
- **Swing Modes** - 4 preset oscillation patterns for automated movement
- **Serial Control** - UART command interface for real-time control

### Smart Recovery System
- ✅ On power-up, motors auto-scan to find home position
- ✅ After homing, automatically restore saved positions
- ✅ Resume swing operations if active before power loss
- ✅ Independent recovery for each motor

### Advanced Features
- Non-blocking operation (all motors move simultaneously)
- Real-time position tracking
- Serial logging for debugging
- Configurable step resolution (default: 1600 steps)
- Individual and group motor control

---

## 🛠 Hardware Requirements

### Components
- **Microcontroller:** ESP32 Development Board
- **Stepper Motors:** 6x NEMA stepper motors (or compatible)
- **Motor Drivers:** 6x Stepper motor drivers (A4988, DRV8825, or similar)
- **Sensors:** 6x IR proximity sensors (for homing)
- **Power Supply:** Appropriate voltage/current for your motors

### Wiring Diagram
```
ESP32          Motor Driver 1     Motor 1
-----          --------------     -------
GPIO 13   -->  STEP              
GPIO 12   -->  DIR               
GPIO 14   -->  (IR Sensor)       

[Repeat for Motors 2-6 with respective pins]
```

---

## 📌 Pin Configuration

| Motor | STEP Pin | DIR Pin | IR Sensor Pin |
|-------|----------|---------|---------------|
| 1     | 13       | 12      | 14            |
| 2     | 27       | 26      | 25            |
| 3     | 21       | 22      | 32            |
| 4     | 15       | 2       | 0             |
| 5     | 4        | 16      | 17            |
| 6     | 5        | 18      | 19            |

> **Note:** Pins can be modified in the code's PIN CONFIG section.

---

## 💾 Installation

### 1. Arduino IDE Setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to `File > Preferences`
   - Add to Additional Board Manager URLs:
```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
   - Go to `Tools > Board > Boards Manager`
   - Search for "ESP32" and install

### 2. Upload Code

1. Clone this repository:
```bash
   git clone https://github.com/yourusername/6-motor-controller.git
   cd 6-motor-controller
```

2. Open `motor_controller.ino` in Arduino IDE

3. Select your ESP32 board:
   - `Tools > Board > ESP32 Dev Module`

4. Select the correct COM port:
   - `Tools > Port > [Your ESP32 Port]`

5. Upload the code:
   - Click the Upload button (→)

### 3. Serial Monitor

- Open Serial Monitor: `Tools > Serial Monitor`
- Set baud rate to **115200**
- You should see initialization messages

---

## 🔧 How It Works

### System Architecture
```
┌─────────────────────────────────────────────────────────┐
│                    POWER ON / RESET                      │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
         ┌────────────────────────────┐
         │   Initialize EEPROM        │
         │   Load Saved States        │
         └────────────┬───────────────┘
                      │
                      ▼
         ┌────────────────────────────┐
         │   Start Auto-Scan          │
         │   (All 6 Motors)           │
         └────────────┬───────────────┘
                      │
         ┌────────────▼────────────────┐
         │  Motors scan back & forth   │
         │  looking for IR sensor      │
         └────────────┬────────────────┘
                      │
                 ┌────▼─────┐
                 │ Sensor   │
                 │Triggered?│
                 └────┬─────┘
                      │ YES
                      ▼
         ┌────────────────────────────┐
         │   6-Phase Homing Sequence  │
         │   Position Reset to 0      │
         └────────────┬───────────────┘
                      │
                      ▼
         ┌────────────────────────────┐
         │   Restore Saved Position   │
         │   Restore Swing Mode       │
         │   Resume Operation         │
         └────────────────────────────┘
```

### EEPROM State Management

The controller saves three key values for each motor:
1. **Current Position** (0-1600 steps)
2. **Swing Mode** (0-4)
3. **Swing Active** (true/false)

**When saved:**
- After each movement completion
- When motor stops
- When swing mode changes
- After homing sequence

**When restored:**
- After power-up homing completes
- Automatically moves to saved position
- Resumes swing if it was active

---

## 📖 Usage

### Basic Workflow

1. **Power On System**
   - Motors begin auto-scanning
   - Serial monitor shows initialization

2. **Wait for Homing**
   - Each motor finds its sensor
   - Completes 6-phase homing sequence
   - Position set to 0 (home)

3. **Automatic Restoration**
   - Motors move to saved positions
   - Swing modes resume if active

4. **Send Commands**
   - Control via Serial Monitor
   - Individual or group control

### Example Session
```
=== 6-MOTOR CONTROLLER WITH EEPROM ===
EEPROM: Valid data found
Loading motor states from EEPROM...
EEPROM: Motor 1 loaded state (Pos=800, Swing=3) - will restore after sensor trigger
EEPROM: Motor 2 loaded state (Pos=400, Swing=0) - will restore after sensor trigger
...
Starting auto-scan for all motors...
System ready — motors scanning until sensor trigger, then restoring state

Motor 1 SENSOR TRIGGERED
Motor 1 HOME COMPLETE
Motor 1 restoring saved state → moving to position 800
Motor 1 state restoration complete
```

---

## 🎮 Serial Commands

### Global Commands (All Motors)

| Command | Description | Example |
|---------|-------------|---------|
| `ALL STOP` | Stop all motors immediately | `ALL STOP` |
| `ALL HOME` | Home all motors | `ALL HOME` |
| `ALL G[%]` | Move all motors to percentage | `ALL G50` |
| `ALL SW[mode]` | Set swing mode for all | `ALL SW3` |

### Individual Motor Commands

| Command | Description | Example |
|---------|-------------|---------|
| `M[N] STOP` | Stop motor N | `M1 STOP` |
| `M[N] HOME` | Home motor N | `M3 HOME` |
| `M[N] G[%]` | Move motor N to percentage | `M2 G75` |
| `M[N] SW[mode]` | Set swing mode for motor N | `M4 SW2` |

**Motor Numbers:** 1-6

**Position Range:** 0-100%
- 0% = Home position (0 steps)
- 100% = Full extension (1600 steps)
- 50% = Middle position (800 steps)

---

## 🔄 Swing Modes

Swing modes create automatic oscillating movements:

| Mode | Range | Description |
|------|-------|-------------|
| 0 | - | **OFF** - No swing, manual control only |
| 1 | 0% ↔ 50% | **Half Range** - Small oscillation |
| 2 | 50% ↔ 80% | **Upper Range** - Medium oscillation |
| 3 | 50% ↔ 100% | **Upper Half** - Large oscillation |
| 4 | 0% ↔ 100% | **Full Range** - Maximum oscillation |

### Swing Behavior
- Motors automatically move between two positions
- Direction reverses at each endpoint
- Continues until mode changed or motor stopped
- State saved to EEPROM (resumes after power loss)

### Example Usage
```
M1 SW4        // Motor 1 swings full range (0% to 100%)
ALL SW2       // All motors swing 50% to 80%
M3 SW0        // Stop swing on motor 3
```

---

## 💾 EEPROM Memory Layout

### Structure
```
Address | Size | Content
--------|------|---------------------------
0-1     | 2B   | Magic Number (0xA5B6)
2-13    | 12B  | Motor 1 State
14-25   | 12B  | Motor 2 State
26-37   | 12B  | Motor 3 State
38-49   | 12B  | Motor 4 State
50-61   | 12B  | Motor 5 State
62-73   | 12B  | Motor 6 State
```

### Per-Motor State (12 bytes)
```
Offset | Size | Field
-------|------|-------------------
+0     | 4B   | Current Position (int)
+4     | 4B   | Swing Mode (int)
+8     | 4B   | Swing Active (bool as int)
```

### Magic Number
- **Purpose:** Validates EEPROM has been initialized
- **Value:** 0xA5B6
- **Function:** Distinguishes valid data from random/corrupt values

---

## ⚡ Power Recovery Sequence

### Detailed Flow

1. **System Startup**
```
   ├── Initialize EEPROM
   ├── Check Magic Number
   │   ├── Valid → Load saved states
   │   └── Invalid → Initialize defaults
   └── Start auto-scan on ALL motors
```

2. **Per-Motor Recovery** (Independent)
```
   ├── Motor scans (forward/backward)
   ├── IR Sensor triggered
   ├── 6-Phase homing sequence
   │   ├── Phase 1: Move forward 20 steps
   │   ├── Phase 2: Move backward 20 steps
   │   ├── Phase 3: Set position = 0, wait 200ms
   │   ├── Phase 4: Move forward 800 steps
   │   ├── Phase 5: Wait 200ms
   │   └── Phase 6: Move backward 800 steps
   ├── Position confirmed at 0
   ├── Check for saved state
   │   ├── If exists → Move to saved position
   │   └── Restore swing mode
   └── Resume normal operation
```

3. **Timeline Example**
```
   T=0s    Power on, load EEPROM states
   T=1s    All motors scanning
   T=3s    Motor 1 sensor triggers
   T=4s    Motor 1 homing complete
   T=5s    Motor 1 moving to saved position (800 steps)
   T=6s    Motor 1 at saved position, swing resumed
   T=7s    Motor 3 sensor triggers
   ...     (continues independently for each motor)
```

---

## ⚙️ Technical Specifications

### Motor Control
- **Step Resolution:** 1600 steps per full range
- **Step Pulse Width:** 1000 microseconds (500 Hz)
- **Position Range:** 0-1600 steps (0-100%)
- **Control Method:** Non-blocking parallel execution

### Timing Parameters
- **Homing Wait Time:** 200ms between phases
- **Homing Movement:** 20 steps (sensor clear), 800 steps (calibration)
- **Swing Delay:** 10ms minimum between movements
- **State Restoration Delay:** 1000ms after homing

### Memory Usage
- **EEPROM:** 512 bytes allocated
- **Used:** ~74 bytes (magic + 6 motors × 12 bytes)
- **Available:** ~438 bytes for future expansion

### Performance
- **Motors:** 6 simultaneous
- **Update Rate:** Microsecond-precision timing loop
- **Serial Baud Rate:** 115200
- **Command Processing:** Real-time, non-blocking

---

## 🐛 Troubleshooting

### Motor Not Moving

**Symptoms:** Motor doesn't respond to commands

**Solutions:**
1. Check wiring connections (STEP, DIR, ENABLE pins)
2. Verify motor driver is powered
3. Check motor driver ENABLE pin (should be LOW)
4. Verify pin configuration matches your hardware
5. Check serial monitor for error messages

### Motor Not Homing

**Symptoms:** Motor scans continuously, never triggers sensor

**Solutions:**
1. Test IR sensor separately (should read LOW when triggered)
2. Check sensor power connections (VCC, GND)
3. Adjust sensor sensitivity/distance
4. Verify IR_PIN configuration
5. Check sensor alignment with trigger target

### EEPROM Not Saving

**Symptoms:** Positions not restored after power cycle

**Solutions:**
1. Check serial output for "EEPROM: Motor X saved" messages
2. Verify `EEPROM.commit()` is called after writes
3. Increase EEPROM_SIZE if using many motors
4. Clear EEPROM and reinitialize:
```cpp
   // Add to setup() temporarily
   for(int i=0; i<EEPROM_SIZE; i++) EEPROM.write(i, 0xFF);
   EEPROM.commit();
```

### Wrong Position After Recovery

**Symptoms:** Motor moves to incorrect position after power-up

**Solutions:**
1. Ensure homing completes before restoration
2. Check saved position is within valid range (0-1600)
3. Verify `currentPos` updates correctly during movement
4. Manually home motor and set position: `M1 HOME`

### Serial Commands Not Working

**Symptoms:** Commands sent but nothing happens

**Solutions:**
1. Check baud rate is 115200
2. Verify newline character is sent (Arduino IDE: "Newline")
3. Commands are case-insensitive but check format
4. Look for "CMD: [your command]" echo in serial monitor
5. Try simple command first: `ALL STOP`

### Multiple Motors Conflict

**Symptoms:** Motors interfere with each other

**Solutions:**
1. This shouldn't happen with non-blocking design
2. Check for sufficient power supply (motors share power)
3. Verify unique pin assignments (no duplicates)
4. Ensure ground is common across all components

---

## 🔌 Wiring Best Practices

### Power Supply
- Use adequate power supply for all motors (calculate: V × I × 6)
- Add bulk capacitors (100-1000µF) near motor drivers
- Separate logic and motor power if possible

### Signal Integrity
- Keep STEP/DIR wires short and away from power wires
- Use twisted pair or shielded cables for long runs
- Add pull-down resistors (10kΩ) on STEP/DIR if noise issues

### Sensor Connections
- Use shielded cable for IR sensor signals
- Add 0.1µF ceramic capacitor across sensor power pins
- Keep sensor power separate from motor power

---

## 🎯 Configuration Options

### Modify Step Resolution
```cpp
// Change steps per full range (line ~44-49)
const int FULL_STEPS_1 = 3200;  // For 1/16 microstepping
const int FULL_STEPS_2 = 3200;
// ... etc
```

### Adjust Motor Speed
```cpp
// Change step pulse width in microseconds (line ~51)
const unsigned long STEP_HALF_US = 500UL;  // Faster (1000 Hz)
const unsigned long STEP_HALF_US = 2000UL; // Slower (250 Hz)
```

### Modify Homing Behavior
```cpp
// In processHomePhases() function
// Phase 1: Change clearance distance
startMoveSteps(id, 40, true);  // 40 steps instead of 20

// Phase 4: Change calibration distance  
startMoveSteps(id, 1600, true); // Full range instead of 800
```

---

## 📊 Example Projects

### 1. Automated Camera Slider
- 2 motors for X-Y movement
- Swing mode for timelapse panning
- Position restore for consistent shots

### 2. Multi-Axis Laser Engraver
- 6 motors control mirrors/galvanometers
- Precise positioning with EEPROM
- Resume engraving after power loss

### 3. Robotic Arm
- 6 motors for 6-DOF arm
- Save preset positions
- Swing modes for testing/demonstration

### 4. Automated Curtain System
- 6 separate curtains/blinds
- Scheduled movements with swing
- Power recovery maintains positions

---

## 🚀 Future Enhancements

Potential improvements for this project:

- [ ] Add acceleration/deceleration profiles
- [ ] Implement speed control per motor
- [ ] Web interface for remote control
- [ ] WiFi/Bluetooth command support
- [ ] Save multiple position presets
- [ ] Add limit switches (min/max)
- [ ] Implement current monitoring
- [ ] Add motor stall detection
- [ ] Create configuration file (JSON)
- [ ] Develop GUI control application

---

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

1. **Fork the repository**
2. **Create a feature branch**
```bash
   git checkout -b feature/amazing-feature
```
3. **Commit your changes**
```bash
   git commit -m 'Add some amazing feature'
```
4. **Push to the branch**
```bash
   git push origin feature/amazing-feature
```
5. **Open a Pull Request**

### Contribution Guidelines
- Follow existing code style
- Add comments for complex logic
- Test on actual hardware before submitting
- Update README if adding features
- Include example usage in PR description

---

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
```
MIT License

Copyright (c) 2025 [Your Name]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 👨‍💻 Author

**[Your Name]**
- GitHub: [@yourusername](https://github.com/yourusername)
- Email: your.email@example.com

---

## 🙏 Acknowledgments

- ESP32 Community for excellent documentation
- Arduino Core Team for ESP32 support
- Contributors to stepper motor control libraries
- Open-source community for inspiration

---

## 📞 Support

If you have questions or need help:

1. **Check the [Troubleshooting](#troubleshooting) section**
2. **Open an [Issue](https://github.com/yourusername/6-motor-controller/issues)**
3. **Join discussions in [Discussions](https://github.com/yourusername/6-motor-controller/discussions)**
4. **Email:** your.email@example.com

---

## ⭐ Star History

If this project helped you, please consider giving it a star! ⭐

[![Star History Chart](https://api.star-history.com/svg?repos=yourusername/6-motor-controller&type=Date)](https://star-history.com/#yourusername/6-motor-controller&Date)

---

## 📈 Project Status

**Current Version:** 1.0.0  
**Status:** Active Development  
**Last Updated:** January 2025

### Changelog

#### v1.0.0 (January 2025)
- Initial release
- 6-motor parallel control
- EEPROM state persistence
- Automatic position recovery
- Serial command interface
- 4 swing modes
- IR sensor homing

---

**Made with ❤️ for the maker community**
