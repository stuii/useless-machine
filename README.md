# Useless machine

A switch-flipping box with a personality: one servo arm, an 8x8 RGB face, and a passive piezo. Repeated poking changes its mood and behavior; leaving it alone lets it recover and fall asleep.

The firmware is C++ for a classic 5 V, 16 MHz ATmega328P Arduino Nano clone, built with PlatformIO in VS Code.

## Hardware and wiring

[View the schematic](docs/schematic.png) | [Scalable SVG](docs/schematic.svg)

| Component | Connection |
|---|---|
| Maintained switch | D4 to switch to GND; internal pull-up, LOW means ON |
| Arm servo | Signal on D9; power from the 5 V rail |
| 8x8 WS2812B grid | DIN on D7 through a 330-470 ohm resistor; power from the 5 V rail |
| Passive piezo | D8 through 330 ohms to positive lead; other lead to GND |

Connect all grounds together. Use an external regulated 5 V supply sized for the servo's stall current plus the grid. Feed the servo and grid directly from the supply rails, with short power connections. Place a 500-1000 uF capacitor across the grid supply near its connector, observing polarity and using a voltage rating above 5 V.

For USB programming, keep the external positive rail separate from the USB-powered Nano's 5V pin unless power isolation is verified. For standalone operation, the regulated rail can also feed the Nano's 5V pin. Do not feed regulated 5 V through VIN or route servo current through the Nano's USB connection or regulator.

The firmware holds the onboard D13 LED LOW. The power LED and bootloader activity are independent of the sketch.

## Build and upload

1. Install VS Code and the PlatformIO IDE extension.
2. Open this folder. PlatformIO installs the AVR platform and the pinned libraries: Servo 1.2.2 and Adafruit NeoPixel 1.12.3.
3. Run **PlatformIO: Build**, then **PlatformIO: Upload** with the Nano connected.
4. Open **PlatformIO: Serial Monitor** at 115200 baud.

The configured environment is `nanoatmega328new`. From a PlatformIO terminal:

```sh
pio run -e nanoatmega328new
pio run -e nanoatmega328new -t upload
pio device monitor -b 115200
```

Ports are auto-detected. If needed, set `upload_port` and `monitor_port` under `[env]` in [platformio.ini](platformio.ini). Use `pio device list` to find the port, and close other serial applications before uploading.

## Operation and calibration

The arm rests at **180 degrees** and pushes the switch at **60 degrees**. These endpoints and other settings are in [include/config.h](include/config.h).

On power-up, the arm is commanded home while a silent 2.4-second rainbow animation fills the grid and fades into the sleeping face. The machine starts asleep. A switch already ON at startup triggers one response after the intro.

Each accepted ON starts a mood-specific sound and waits **400 ms** before issuing an arm movement. The response can include gestures and additional pauses before the push. A stable OFF stops the sound, cancels pending gestures or jabs, and retracts the arm. Switch debounce is **30 ms**. A fresh OFF-to-ON can interrupt the current response.

Actual pushes command 60 degrees directly, without a software speed ramp. Each push is held for at most 500 ms; returning home includes 500 ms settling. A switch held ON does not trigger endless retries. The arm stays attached at home, including during sleep.

The firmware tracks commanded angles, not shaft position or force. Check linkage clearance when calibrating: preliminary gestures must remain short of switch contact, and neither endpoint should force the servo against a mechanical stop.

## Moods and movement

Each active mood has three rotating movement sequences. Its rotation starts at variant 2 and continues 3, 1, 2. The serial monitor prints the selected variant. Special interaction tricks can take precedence over these sequences.

| Mood | Variant 1 | Variant 2 | Variant 3 |
|---|---|---|---|
| Happy | Friendly bob, retract, push | Two greeting bobs at different heights | Bow forward, wind back, swat |
| Curious | Inspect at 108 degrees, move closer to 90, then push | Approach, retreat, look closer, retreat again | Three-step approach with a second inspection |
| Suspicious | Approach 108 degrees, withdraw to 150, hesitate, lunge | Startle, hide, cautiously reappear | Abort two approaches, hide, then return |
| Playful | Dart to 80 degrees, pause, hide, slam | Two 80-degree fake-outs and a small peek | Hide for a long beat, peek twice, slam |
| Annoyed | Single swat | Two impatient air-knocks, then swat | Stare near the switch, withdraw, swat |
| Furious | Swat, with a second jab if still ON | Short wind-up shake before attacking | Charge, brake, recoil, attack |
| Tired | Slow creep, pause, slam | Two sleepy nods before creeping | Two droopy false starts before creeping |

Tired always creeps to **100 degrees at 18 ms per degree**, pauses for **900 ms**, then slams to 60 degrees and retracts slowly. If interrupted at or beyond the creep stop, it returns home before starting that approach again.

Furious allows at most two actual pushes per activation. If the switch remains ON after the first push, it recoils to 108 degrees, waits 300 ms, and jabs again. OFF cancels the second jab. Other routines make one actual push.

Sleepy and asleep are resting expressions. An activation wakes the personality and selects its response from the updated parameters.

## Interaction tricks

- **You win:** if the switch stays ON after the allowed attempts (one push, or up to two for furious), the arm retracts fully and the face shows downcast eyes, blushing cheeks, a crooked mouth, and a tiny waving white flag. It stays silently surrendered at home until OFF, with no further pushes. Turning OFF during retraction cancels the surrender; a successful push follows the victory path instead. The serial monitor prints `You win; returning home` when the attempt budget is exhausted.

- **I saw that coming:** every third consecutive quick re-flip earns a knowing wink, a dart to 80 degrees, a 550 ms stare, then a push. A quick re-flip means ON within 1.2 seconds of OFF; a slower one resets the streak. Tired keeps its creep routine.
- **Fake sleep:** every second playful encounter stays home with closed eyes for 1.3 seconds, opens one eye for 650 ms, then slams. Anticipation takes precedence if both qualify.
- **Victory celebrations:** an OFF edge while holding a push qualifies for a celebration after retraction. The first three celebrations per power-up are wink/confetti, a pulsing heart, and colorful border confetti. Each lasts 1.4 seconds, stays silent, and keeps the arm home. Another ON interrupts it.
- **Dreaming:** after 12 seconds continuously asleep, a dim heart, star, or switch fades above the sleeping face for 3.2 seconds. Dreams repeat every 18 seconds, cycling through the pictures. Moving Zs return between dreams. The arm stays still and the piezo stays silent.

### Rare surprises

After the first four interactions, eligible pokes have a 1-in-12 chance of replacing the usual routine with one of these performances:

- **Rainbow system error:** colorful broken scanlines for 1.3 seconds, a brief composed face, then a single push at 1.8 seconds. This is an animation, not a reset.
- **Dramatic countdown:** the grid counts 3, 2, 1 in changing colors, spending 700 ms on each digit before the slam.
- **Deadpan:** flat eyes and a straight mouth, an unblinking 1.1-second stare at home, then one plain push.

The arm stays home during the performance. Each retains the 400 ms sound lead-in and uses the normal interaction sound. OFF cancels the pending push; another ON replaces the performance. Tired, anticipation, and fake sleep take priority.

After a surprise, at least eight intervening interactions and 45 seconds must pass before another can be selected. Consecutive surprises never use the same performance. Selection mixes interaction timing into a private pseudorandom generator; it does not run on a fixed flip count. Serial output identifies the selection as `Rare surprise=1/2/3` in the order above.

## Personality and recovery

Six parameters range from 0 to 100. Repeated poking means another activation before eight seconds of quiet rest; this is separate from the 1.2-second anticipation window.

| Parameter | Initial value | Interaction | Quiet recovery per second |
|---|---|---|---|
| Energy | 90 | -4 spaced; -8 repeated | +4 |
| Annoyance | 0 | +3 spaced; +10 to +15 repeated, depending on patience | -20 |
| Excitement | 0 | +20 spaced; +24 repeated | -4 |
| Curiosity | 40 | +22 spaced; -12 repeated | +2 when below 70, capped at 70 |
| Patience | 80 | -3 spaced; -10 repeated | +5 |
| Mischief | 25 | +16 spaced or +8 repeated while annoyance is below 55; otherwise -12 | Moves 3 toward 25 |

Recovery requires the arm to be idle and the switch OFF. Excitement decreases each quiet second; the other changes start at two quiet seconds. Holding ON or running a movement prevents recovery.

Annoyance at 55 selects annoyed; at 75 it selects furious. Energy at or below 25 selects tired ahead of anger. Playful needs at least three pokes in a streak, excitement of at least 60, and mischief of at least 35; higher-priority moods can still win. Eight quiet seconds reset the streak on the next activation.

With suitable calm parameters, 30 quiet seconds lead to sleepy and 60 to asleep. Personality resets on power-up and is not stored in EEPROM. The serial monitor reports the mood and all six parameters on activation, on mood changes, and every five seconds.

See [the personality flowchart](docs/personality-flowchart.md) for the full mood priorities and behavior paths.

## Face and sound

The grid displays nine mood expressions with blinking, breathing, and eye-close/blend/eye-open transitions. Sleep has two staggered 4x4 Zs moving upward every 160 ms. They clip at the panel edge and keep a fixed dim blue color independent of breathing.

The panel uses straight rows, no rotation, and no mirroring. Brightness is **10/255**. Configuration options in `include/config.h` include:

| Setting | Purpose |
|---|---|
| `FACE_BRIGHTNESS` | Overall LED brightness |
| `MATRIX_SERPENTINE` | Straight or alternating row wiring |
| `MATRIX_ROTATION` | Clockwise quarter-turns, 0 to 3 |
| `MATRIX_MIRROR` | Horizontal mirroring |
| `SLEEP_Z_STATIC_TEST` | Freeze Z motion for orientation checks |
| `SOUND_ENABLED` | Enable or mute the piezo |

The passive piezo plays mood-specific phrases with 4 ms pitch updates. Sound starts only on accepted interaction and stops on OFF. Startup, recovery, sleep, dreams, and celebrations are silent.

Servo timing uses Timer1; Arduino `tone` uses Timer2. WS2812B transfers start within the 4-14 ms portion of the servo's 20 ms frame while D9 is LOW, and face updates avoid active audible tones. This scheduling assumes one Servo 1.2.2 object on the 16 MHz ATmega328P.

## Source layout

| File | Responsibility |
|---|---|
| [src/main.cpp](src/main.cpp) | Switch handling, arm state machine, interaction tricks |
| [include/config.h](include/config.h) | Pins, calibration, timings, thresholds |
| [include/personality.h](include/personality.h) | Parameters, recovery, mood selection |
| [include/choreography.h](include/choreography.h) | Gesture sequences stored in flash |
| [src/face.cpp](src/face.cpp) | Face rendering, transitions, celebrations, dreams |
| [src/sound.cpp](src/sound.cpp) | Nonblocking piezo playback |
| [tests/logic_checks.cpp](tests/logic_checks.cpp) | Compile-time logic checks |
| [tests/host/mechanism_test.cpp](tests/host/mechanism_test.cpp) | Controller simulations with fake Arduino I/O |

## Verification

Build the firmware with PlatformIO. Compile the logic checks with the AVR toolchain available on PATH:

```sh
avr-g++ -mmcu=atmega328p -std=c++14 -Iinclude -c tests/logic_checks.cpp -o .pio/logic_checks.o
```

The host simulator has 86 scenarios covering mood routines, gesture cancellation, switch debounce, held-ON limits, rapid re-flips, tired approaches, startup, timing rollover, anticipation, fake sleep, victory limits, and the dream schedule. With Zig installed and available on PATH, run from PowerShell:

```powershell
zig c++ -target x86_64-windows-gnu -std=c++14 -Itests/host -Iinclude tests/host/mechanism_test.cpp -o .pio/mechanism_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Simulator build failed' }
0..85 | ForEach-Object {
    & ./.pio/mechanism_test.exe $_
    if ($LASTEXITCODE -ne 0) { throw "Simulation failed: $_" }
}
```

Simulations verify controller behavior, not physical travel, torque, current, or LED appearance. Check those on the assembled machine.
