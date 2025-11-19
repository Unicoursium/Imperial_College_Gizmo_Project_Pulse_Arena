# Pulse Arena – 3.0

This version of the tank robot introduces an upgraded control system powered by the PCA9685 PWM driver, enabling more stable, low-latency, and highly precise motor and servo control.  
All gameplay features from Version 2 remain, including movement, turret rotation, IR shooting, and hit detection — but V3 brings a significantly more responsive and smoother control experience.

This readme file is written by ChatGPT initially and refined afterwards!!!

---

## Features

### 1. **Shoot (IR Transmission)**
- Triggered by **ZR / RT** button on the controller.
- Sends an ONKYO-format IR code to attack the opponent tank.

### 2. **Hit Detection (IR Reception)**
- Each tank includes an IR receiver module.
- When a valid IR hit code is detected, the tank responds (LED flash, life deduction, etc.).

### 3. **Turret Rotation (Servo via PCA9685)**
- Controlled by the **Right Joystick (Horizontal Axis)**.
- Additional fine-tuning using **L1 / R1**.
- Smooth rotation using real-time dt-based servo motion.
- Powered by PCA9685 for improved precision and lower jitter compared to direct GPIO.

### 4. **Movement (Tank Differential Drive through PCA9685)**
- Controlled by the **Left Joystick**.
- Supports:
  - Forward / Backward
  - Turning
  - Pivot turning (in-place rotation)
  - Arc-turning (mixed forward + turn)
- PCA9685 drives the H-bridge channels, achieving:
  - Lower latency  
  - Higher resolution  
  - More stable motor output  

---

## Hardware Requirements

ESP32 × 2  
IR Transmitter × 2  
IR Receiver Module × 2  
Bluetooth Controller × 2  
DC Motors × 4  
Servo Motors × 2  
H-Bridge Motor Drivers × 2  
PCA9685 PWM Driver × 2  

---

## Wiring Guide

### **ESP32 → PCA9685 (I2C)**
SDA → GPIO 21  
SCL → GPIO 22  
VCC → 3.3V  
GND → Shared GND  

---

### **PCA9685 → Motors (H-Bridge Channels)**

| Motor Function        | PCA9685 Channel |
|-----------------------|------------------|
| Left Motor Forward    | CH1 |
| Left Motor Backward   | CH3 |
| Right Motor Forward   | CH2 |
| Right Motor Backward  | CH4 |

Motor Driver Power → External power supply  
ESP32 GND → MUST share ground with PCA9685 and motor driver  

---

### **PCA9685 → Turret Servo**
Servo Signal → CH6  
Power → 5V  
Ground → Shared GND  

---

### **ESP32 → IR Module**
IR Transmitter (TX LED) → GPIO 16  
IR Receiver (OUT pin) → GPIO 17  

---

### **Hit Indicator LED**
Hit LED → GPIO 25  
Ground → GND  

---

## Controller Layout

| Controller Input | Function |
|------------------|----------|
| Left Joystick    | Tank movement |
| Right Joystick   | Turret rotation |
| L1 / R1          | Fine-tune turret angle |
| ZR / RT Button   | Fire IR shot |

---

