# Pulse Arena – 2.0

This version of the tank robot introduces a complete movement and combat system using ESP32 microcontrollers, IR communication, motor drivers, and a Bluetooth game controller.  
Two tanks can battle each other by firing IR signals and detecting hits.

This readme file is written by ChatGPT initially and we've refined some parts!!!
---

## Features

### 1. **Shoot (IR Transmission)**
- Triggered by **ZR / RT** button on the controller.
- Sends an ONKYO-format IR code to attack the opponent tank.

### 2. **Hit Detection (IR Reception)**
- Each tank has an IR receiver.
- When a valid IR hit code is received, the tank responds (LED flash, life deduction, etc.).

### 3. **Turret Rotation**
- Controlled by the **Right Joystick (Horizontal Axis)**.
- Smooth rotation using servo motor control.
- Speed-limited for realistic turret movement.

### 4. **Movement (Tank Differential Drive)**
- Controlled by the **Left Joystick**.
- Supports:
  - Forward / Backward
  - Turning
  - Pivot turning (turning in place)
- Uses differential mixing algorithm to drive left and right motor pairs.

---

## Hardware Requirements

ESP32 × 2

IR Transmitter (LED) × 2

IR Receiver Module × 2

Bluetooth Controller × 2

DC Motors × 4

Servo Motors × 2

H-Bridge Motor Drivers × 2


---

## Wiring Guide

### **ESP32 → Motor Driver (H-Bridge)**
Left Motor Forward → GPIO 18

Left Motor Backward → GPIO 19

Right Motor Forward → GPIO 21

Right Motor Backward → GPIO 22

### **Motor Driver Power → Power supply board**

ESP32 GND → MUST share ground with motor driver

### **ESP32 → IR Module**
IR Transmitter (TX LED) → GPIO 16
IR Receiver (OUT pin) → GPIO 17

### **ESP32 → Turret Servo**
Turret Servo Signal → GPIO 5
Servo Power → 5V
Ground → Shared GND

### **Hit Indicator LED**
Hit LED → GPIO 23
Ground → GND

---

## Controller Layout

| Controller Input | Function |
|------------------|----------|
| Left Joystick    | Tank movement |
| Right Joystick   | Turret rotation |
| ZR / RT Button   | Fire IR shot |

---

