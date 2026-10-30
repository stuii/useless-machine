# Useless machine

First breadboard prototype: classic 5 V / 16 MHz ATmega328P Nano clone, one maintained switch on **D4**, one positional hobby servo on **D9**. The final machine will have three switches, an arm servo, a pivot servo, and a lid servo.

## Parts

- Nano clone and a USB **data** cable.
- Standard positional servo (not a continuous-rotation servo), arm/horn, breadboard, jumpers.
- Maintained SPST or SPDT toggle switch. A momentary button can test input but will not stay on for the arm to turn off.
- Regulated external servo supply matching your servo voltage rating; 5 V is common. Choose current capacity above the servo's specified stall current.
- Optional 470–1000 µF electrolytic capacitor across the servo supply near the servo, rated above the supply voltage, with correct polarity. This does not replace a sufficient supply.

## Wiring (power disconnected)

| Connection | Destination |
| --- | --- |
| Nano D4 | One SPST switch terminal, or SPDT common |
| Other SPST terminal / selected SPDT throw | GND (this position is ON) |
| Unused SPDT throw | Leave disconnected |
| Servo signal (often orange/yellow/white) | Nano D9 |
| Servo positive (often red) | External regulated servo supply positive |
| Servo ground (often brown/black) | External supply negative |
| Nano GND | External supply negative / common ground |
| Nano USB | Computer, powering the Nano and carrying uploads |

Check the servo's actual wire assignment. D4 uses INPUT_PULLUP: open = OFF/HIGH, connected to GND = ON/LOW. No external pull-up resistor is needed. Use the pins labelled **D4** and **D9**, not the fourth/ninth physical pins.

Keep the external supply positive separate from the Nano 5V/VIN and USB power. Do not power the servo from a digital pin or rely on the Nano's regulator/USB for servo current. All grounds must connect. Some breadboard power rails are split; check continuity. Keep servo power leads short and use direct power connections if the breadboard contacts cannot carry the servo current.

## VS Code / PlatformIO

1. Install VS Code and the PlatformIO IDE extension (recommended by this project).
2. Open this folder and let PlatformIO install the pinned AVR platform and Servo library.
3. Run **PlatformIO: Build**. The default environment is nano_old, the classic old bootloader.
4. Connect the Nano, then run **PlatformIO: Upload**. If upload fails with a sync error, try the nano_new environment from PlatformIO Project Tasks. This changes bootloader/upload settings, not the wiring.
5. Open **PlatformIO: Serial Monitor** at 115200 baud. Close other serial applications before uploading.

Commands in a PlatformIO terminal:

~~~sh
pio run
pio run -e nano_old -t upload
pio device monitor -b 115200
# Alternate bootloader:
pio run -e nano_new -t upload
~~~

Ports are auto-detected. If multiple serial devices are present, add upload_port and monitor_port (for example COM5) under [env] in platformio.ini. Find the correct port in Device Manager or with pio device list. A clone may need a USB-serial driver matching its actual chip (often CH340); obtain it from the chip vendor. This configuration assumes ATmega328P, not an ATmega168 or another Nano-family MCU.

## First power-up and calibration

1. Remove the horn/linkage so the servo cannot drive into the mechanism. Leave the switch OFF.
2. Upload the firmware; power the servo supply. The servo is commanded to HOME_DEG immediately at startup. The board cannot sense its actual position.
3. In include/config.h, adjust HOME_DEG (initially 60) and PUSH_DEG (initially 110). These are starting guesses, not calibrated limits. Stay within the servo's usable range.
4. Power off before fitting the horn at home. Keep the path clear; start with a small motion and change angles in small increments, rebuilding/uploading each time. PUSH_DEG can be smaller than HOME_DEG if the arm moves the other way.
5. Toggle ON. After 400 ms, the arm steps toward the switch. It returns when OFF is detected, or after reaching the push position and holding for at most 500 ms.
6. If the switch remains ON, it retracts and waits for OFF before accepting another attempt. Adjust geometry/angles before retrying. Disconnect servo power if it stalls or buzzes against a stop; there is no current or force sensor.

A switch already ON at boot will trigger once after startup settling. The servo stays attached at home to hold position. Motion is non-blocking, with 30 ms switch debouncing. Increase STEP_MS for slower movement.

## Breadboard acceptance checks

- OFF at boot: arm homes and remains there.
- ON at boot: one attempt, then home.
- Normal ON: pause, push, switch opens, return home; repeat several times.
- Switch held ON / arm misses: one bounded attempt, then wait for OFF; no repeated pushing.
- ON then OFF during the reaction pause: no extension.
- OFF during extension: retract promptly.
- Watch for resets or jitter during movement; inspect servo power and common ground.

Compilation can verify software, but physical travel, current demand and switch contact geometry must be checked on the real hardware.

## Later expansion

Suggested unused pins: switches 2 and 3 on D5/D6, pivot servo on D10, lid servo on D11. D4/D9 remain as wired. Servo signals do not require hardware PWM pins. The AVR Servo library uses Timer1, so analogWrite PWM on D9/D10 is unavailable while using it.

Extend the current state machine to: select an active switch, open lid, pivot to its calibrated position, extend arm, retract, then close lid. Add separate per-switch pivot positions and mechanical clearance sequencing before attaching the extra servos. Size the external supply and distribution for all servos' combined load; do not run their power through Nano pins.

## References

- [PlatformIO classic Nano](https://docs.platformio.org/en/stable/boards/atmelavr/nanoatmega328.html)
- [PlatformIO new-bootloader Nano](https://docs.platformio.org/en/stable/boards/atmelavr/nanoatmega328new.html)
- [Arduino Nano processor/bootloader selection](https://support.arduino.cc/hc/en-us/articles/4401874304274-Select-the-right-processor-for-Arduino-Nano)
- [Arduino servo troubleshooting and power](https://support.arduino.cc/hc/en-us/articles/360017053760-Troubleshoot-servo-motors)

## Build verification

Both nano_old and nano_new compiled successfully with the pinned dependencies. Each uses 4,112 bytes flash and 247 bytes RAM. Arduino core new.cpp emits unused-parameter warnings with the extra warnings enabled; the project firmware compiles without warnings. Hardware upload and mechanical operation have not been verified.
