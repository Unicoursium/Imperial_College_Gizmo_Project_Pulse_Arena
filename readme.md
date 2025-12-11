# Pulse_Arena Project Code

Hi everyone, this is Unico, and this repos is the Gizmo project Pulse Arena

## Intro
Our project is a tank battle game for family gatherings and parties. The game features two tanks that use infrared cannons to shoot cryptic codes at the weak points of another tank to cause damage, defeating the other tank and winning the game.

## Game mechanics

The battle system centres on two wireless-controlled tanks equipped with infrared cannons, directional mobility, and a dynamic health-bar display. Each tank begins with 10 HP and loses 2 HP when struck by an opponent’s IR shot. Upon reaching 0 HP, the tank enters a Doom State—movement becomes restricted to rotation only, and a secondary Doom-Health bar (10 points) begins flashing. A further two hits (5 HP each) will destroy the tank completely, triggering a game-over broadcast to the server. Players may also eliminate a doomed opponent instantly using the dedicated physical kill-switch. Tanks communicate real-time events to the control server via NRF24, enabling sound effects, scoring, and match-flow management.

## Tank structure

### Shell
Let's thank Stanley, who led the modeling of the tank's hull. We went through three main iterations.

#### iteration 1
the tank only has a base, which it's more likely an RC car

#### iteration 2
we added a shell to the tank, making it more likely a tank, and we've got a turret to send IR code.

#### iteration 3
We've got a modern tank shell. You'll see how cool it is.

### Electronics part
We need

1. ESP x2
2. IR transmitter x2
3. IR receiver x4
4. Controller x2
5. Motor x4
6. Servo x2
7. H bridge x2
8. PCA9685 x2
9. Neopixel x2
10. Power supply board x2
11. NRF24L01 x3
12. Raspberrypi zero 2 w x1
13. Speaker x1

To recreate this tank battle, we need to flash the code into the ESP32 and also have specific code running on the Raspberry Pi.

check src in Pulse_Arena_V4 folder