# 2-DC-Stepper-Motor-Independent-and-Parallel-Control-System-using-UART-Commands
# ESP32 6-Vent Control System
This project controls 6 ventilation motors independently using an ESP32, UART communication, H-bridge motor drivers, and limit sensors.

## Features
- 6 vents controlled fully independently
- UART command interface
- Non-blocking control using `millis()`
- State machine logic for each vent
- Safety timeout to protect motors
- Stop automatically using upper/lower limit sensors

## Hardware Required
- ESP32
- 6 × DC motors
- 6 × H-bridge drivers (L298N / L293D / BTS7960 / MX1508)
- 12 limit switches or 6 × U-type opto sensors
- Power supply 12V for motors
- USB/5V for ESP32

## Wiring Overview
Each vent has:
- 2 motor driver pins (IN1, IN2)
- 2 sensors (UP, DOWN)
- Shared GND between ESP32, sensors, and motor driver

## UART Commands

Examples:

## State Machine Per Vent
Each vent has 3 states:
- IDLE
- OPENING
- CLOSING

The ESP32 loop checks each vent state and:
- Runs motor forward (OPENING)
- Runs motor reverse (CLOSING)
- Stops motor when a sensor triggers
- Uses `millis()` for a safety timeout

## How `millis()` is used
`millis()` gives the time since ESP32 started.
We save:
- start time
- timeout limit (e.g., 7000 ms)

This prevents blocking delays and allows all 6 motors to run simultaneously.

## Workflow Summary
1. UART command sets vent state.
2. State machine decides what motor should do.
3. Sensors stop the motion at limits.
4. `millis()` ensures non-blocking operation and safety timeout.
5. All vents operate independently in the same loop.

## Advantages
- Smooth multi-vent operation
- No blocking delays
- Safe motor control
- Simple UART interface
- Easy to scale or modify

## Notes
You must define your own pins in the code.
Make sure motor power and ESP32 power share the same GND.

