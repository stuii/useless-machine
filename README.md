# Useless machine

First breadboard prototype: classic 5 V / 16 MHz ATmega328P Nano clone, one maintained switch on **D4**, two positional hobby servos (arm **D9**, lid **D11**), an 8×8 WS2812B face on **D7**, and a passive piezo on **D8**. The machine uses one switch and two servos; D5, D6 and D10 are unused.

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
| Arm servo signal | Nano D9 |
| Lid servo signal | Nano D11 |
| Servo positive (often red) | External regulated servo supply positive |
| Servo ground (often brown/black) | External supply negative |
| Nano GND | External supply negative / common ground |
| Nano USB | Computer, powering the Nano and carrying uploads |
| Nano D8 | 330 ohm series resistor → passive piezo + |
| Passive piezo − | Nano/common GND |

Check the servo's actual wire assignment. D4 uses INPUT_PULLUP: open = OFF/HIGH, connected to GND = ON/LOW. No external pull-up resistor is needed. Use the pins labelled **D4** and **D9**, not the fourth/ninth physical pins.

Keep the external supply positive separate from the Nano 5V/VIN and USB power. Do not power the servo from a digital pin or rely on the Nano's regulator/USB for servo current. All grounds must connect. Some breadboard power rails are split; check continuity. Keep servo power leads short and use direct power connections if the breadboard contacts cannot carry the servo current.

## VS Code / PlatformIO

1. Install VS Code and the PlatformIO IDE extension (recommended by this project).
2. Open this folder and let PlatformIO install the AVR platform and the pinned Servo and Adafruit NeoPixel libraries.
3. Run **PlatformIO: Build**. The configured environment is nanoatmega328new (new bootloader), matching your current project settings.
4. Connect the Nano, then run **PlatformIO: Upload**. If your board needs the old bootloader, change board to nanoatmega328 in both places in platformio.ini. This changes bootloader/upload settings, not the wiring.
5. Open **PlatformIO: Serial Monitor** at 115200 baud. Close other serial applications before uploading.

Commands in a PlatformIO terminal:

~~~sh
pio run
pio run -e nanoatmega328new -t upload
pio device monitor -b 115200
~~~

Ports are auto-detected. If multiple serial devices are present, add upload_port and monitor_port (for example COM5) under [env] in platformio.ini. Find the correct port in Device Manager or with pio device list. A clone may need a USB-serial driver matching its actual chip (often CH340); obtain it from the chip vendor. This configuration assumes ATmega328P, not an ATmega168 or another Nano-family MCU.

## First power-up and calibration

1. Remove the horn/linkage so the servo cannot drive into the mechanism. Leave the switch OFF.
2. Upload the firmware; power the servo supply. Startup first opens the lid, then commands the arm to HOME_DEG, then closes the lid. The board cannot sense its actual position.
3. In include/config.h, HOME_DEG is 180° (resting/sleep position) and PUSH_DEG is 60° (switch actuation), configured for the assembled box. Stay within the servo's usable range.
4. Power off before fitting the horn at home. Keep the path clear; start with a small motion and change angles in small increments, rebuilding/uploading each time. PUSH_DEG can be smaller than HOME_DEG if the arm moves the other way.
5. Toggle ON. After a mood- and energy-dependent pause, the arm steps toward the switch. It returns when OFF is detected, or after reaching the push position and holding for at most 500 ms.
6. If the switch remains ON, it retracts and waits for OFF before accepting another attempt. Adjust geometry/angles before retrying. Disconnect servo power if it stalls or buzzes against a stop; there is no current or force sensor.

A switch already ON at boot will trigger once after startup settling. The servo stays attached at home to hold position. Motion is non-blocking, with 30 ms switch debouncing. ARM_STEP_PERCENT scales ramped mood movement intervals: 10 (current) reduces them to one tenth, with a 3 ms minimum, while 100 restores the original speed. STEP_MS sets the happy base interval.

## Breadboard acceptance checks

- OFF at boot: arm homes and remains there.
- ON at boot: one attempt, then home.
- Normal ON: pause, push, switch opens, return home; repeat several times.
- Switch held ON / arm misses: one bounded attempt, then wait for OFF; no repeated pushing.
- ON then OFF during the reaction pause: no extension.
- OFF during extension: retract promptly.
- Watch for resets or jitter during movement; inspect servo power and common ground.

Compilation can verify software, but physical travel, current demand and switch contact geometry must be checked on the real hardware.

## Face and personality

Connect **D7 through a 330–470 ohm resistor to DIN** on the grid, not DOUT. Connect grid +5V to an external regulated 5V supply and grid GND to common ground (Nano GND and servo supply negative). Add a 500–1000 µF capacitor across the grid supply near the panel, with correct polarity and a voltage rating above 5V. Keep external +5V separate from the USB-powered Nano's 5V pin. Power off before wiring.

Use a supply sized for the grid plus servo stall current if they share it, and suitable power wiring; do not feed the panel through the Nano. A conservative full-white budget for 64 older WS2812B pixels is about 3.8 A before the servo. This firmware lights only face pixels at brightness 24/255, so normal draw is much lower; brightness limiting is not a hardware current limiter. See [Adafruit's wiring guidance](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices).

The personality tracks six values from 0 to 100. Energy is the only stamina parameter; the separate tiredness counter has been removed.

| Parameter | What changes it | How it affects behavior |
| --- | --- | --- |
| Energy | Starts at 90; each accepted ON costs 4; recovery adds 4/second | Lower energy slows movement/reactions; 25 or below produces tiredness |
| Annoyance | An isolated poke adds 3; rapid repeats add 6–12 depending on patience; recovery removes 12/second | 55+ is annoyed; 85+ is furious unless exhausted |
| Excitement | +20 for an isolated poke, +24 for a rapid repeat; quiet removes 4/second | Short-lived arousal enables curious/playful moods |
| Curiosity | Starts at 40; isolated pokes add 22, rapid repeats subtract 12; recovery adds 2/second toward 70 | With excitement, 45+ gives an inquisitive face and a longer inspecting pause |
| Patience | Starts at 80; isolated pokes cost 3, rapid repeats cost 10; recovery adds 5/second | Controls irritation from the next rapid poke; independent of existing anger |
| Mischief | Starts at 25; enjoyable pokes add 8–16, irritating pokes subtract 12; recovery moves it 3/second toward 25 | At 35+ with excitement, selects playful mood; fake-out timing varies across three activation variants |

A rapid repeat means another accepted ON before eight seconds of quiet rest. Recovery starts after **2 seconds continuously idle and OFF**. After 30 seconds, a calm machine becomes sleepy; after 60 seconds, it sleeps if annoyance has fallen below 20. A fully depleted machine takes about 26 seconds of uninterrupted quiet to fully restore energy; curiosity fully rebuilds from zero by about 36 seconds. Sleep is a face/behavior state, not an electrical low-power mode; the switch remains responsive and the servo holds home.

Rapid poking usually produces curious → playful → annoyed → furious → tired (suspicion depends on curiosity); playful responses appear when excitement and mischief are high without much irritation. Widely spaced pokes produce a gentler personality. Low energy takes priority over fury: an exhausted machine moves reluctantly instead of getting faster forever. Its reaction and one-degree movement interval are selected once per activation, incorporating energy, curiosity and mischief, with separate speeds for approach, push and return. A fresh switch activation can interrupt the current routine. Endpoints and the bounded push hold remain unchanged.

Each accepted ON counts once, including an activation cancelled during hesitation. A stuck ON does not cause repeated attempts or repeated emotional penalties, and it prevents recovery. Startup resets the personality; values are not written to EEPROM. The serial monitor prints mood, energy, annoyance, excitement, curiosity, patience and mischief on activation, mood changes, and every five seconds.

### Arm response routines

| Mood | Motion |
| --- | --- |
| Happy | Immediate push and return target commands |
| Curious | Creep to 60% travel at 6 ms/degree, pause 120 ms, then slam and snap back |
| Suspicious | Creep to 60% travel at 8 ms/degree, pause 250 ms, then slam and snap back |
| Playful | Open, dart to 80°, pause 700 ms, retract/close, wait 650 ms, reopen and slam |
| Annoyed | No hesitation; direct target command for push and return |
| Furious | No hesitation; direct target command for push and return |
| Tired | Creep to 100° at 18 ms/degree, pause 900 ms, then direct push to 60°; relaxed 7–8 ms/degree retreat |

Suspicion appears with mild irritation (15–54), curiosity at least 45 and excitement at least 10, before outright annoyance. It has a separate purple side-eye face and questioning sound.

A new debounced OFF→ON is accepted during return or home settling. If it occurs within **1.2 seconds of OFF**, it skips the reaction and peek pauses. It reverses immediately only when the lid is fully open and settled. During lid closing it first retracts/settles the arm and reopens the lid. Energy still determines movement speed. The normal 30 ms debounce remains; very short contact changes are filtered. A held ON never repeats an attempt. OFF during approach/pause cancels the push and retracts.

The normal peek position is calculated between HOME_DEG and PUSH_DEG and also works for reversed servo mounting. Tired instead uses TIRED_PEEK_DEG = 100°, with TIRED_CREEP_MS = 18 and TIRED_PAUSE_MS = 900. Confirm this peek position is clear of switch contact. Calibrate it so 60% of travel is still clear of the switch. Servo commands remain inside the calibrated endpoints; actual speed depends on the servo, load and its 20 ms pulse frame. Firmware tracks commanded position, not measured arm position. Check the faster routines on the breadboard.

Tune routines in Model::behavior() in include/personality.h and QUICK_REPEAT_MS / approachAngle in include/interaction.h. Check rapid OFF→ON while retracting, during settling, and with reversed endpoints; also confirm that a missed push with ON held does not loop.

### Startup and sleeping animation

Power-up starts the personality asleep. A silent 1.6-second animation expands and contracts a small blue diamond, then fades in the sleeping face. The lid opens during the display intro; arm homing and lid closure follow before interactions are enabled. A switch already ON is accepted after startup; subsequent accepted activations wake the personality normally.

While asleep, the eyes and mouth sit in the lower half of the matrix, leaving room for one complete 4×4 Z that drifts upward and sideways every 220 ms. The face continues its gentle breathing animation. Sleep and startup never trigger audio. The animations do not block switch sampling or servo pulses.

Animation geometry/timing is in include/sleep_animation.h; startup duration is covered by STARTUP_SETTLE_MS in include/config.h. Check startup with the switch both OFF and ON, then allow it to fall asleep again and verify that the Zs stay above the face. The older contact sheet below shows static expressions; the sleeping layout now reserves its upper half for animation.

### New face designs

![Eight face expressions](docs/faces.png)

Left to right: **happy, curious, playful, annoyed, furious, tired, sleepy, asleep**. The designs use mint, sky blue, lavender, amber, coral and sleepy blues, with eye highlights, subtle rosy cheeks and small mouths. Calm expressions blink at slightly varied intervals; tired blinks are longer; fury stares; sleeping gently dims and brightens. Mood changes use nine animation frames: the old eyes close, the closed expressions and colors crossfade, then the new eyes open. The nominal duration is 320 ms, with frames at least 40 ms apart; audio/servo scheduling can extend it. Frames advance only after reaching the LEDs. If another mood arrives mid-transition, the current transition completes before moving to the latest mood. Normal blinking resumes afterward. Tune FRAME_MS in include/face_transition.h. The source artwork is in include/face_art.h; each character represents one LED and its color role.

In include/config.h, adjust FACE_BRIGHTNESS, MATRIX_SERPENTINE, MATRIX_ROTATION (0–3 clockwise quarter turns), and MATRIX_MIRROR. This panel uses straight/progressive rows: MATRIX_SERPENTINE=false. The supplied video showed alternate Z rows reversed with the previous zigzag setting. Logical pixel zero is upper left, with each row left-to-right; rotation/mirror settings handle mounting orientation. Use MATRIX_SERPENTINE=true only for a different panel with alternating row directions. Rotate/mirror to match the installed panel; color order is NEO_GRB in src/face.cpp (change to NEO_RGB only if your panel swaps red and green).

Face updates are sent only when the expression or animation frame changes. On this Nano, sending WS2812B data masks interrupts for about 2 ms. The firmware places each transfer in the two servos' pulse-free portion of its 20 ms frame to avoid stretching a servo pulse. This is specific to exactly TWO Servo objects, Servo 1.2.2 and 16 MHz ATmega328P. Transfers start at 6–14 ms within the 20 ms frame, after the two possible 2.4 ms pulses, and only when all servo signals are LOW. Rework the window before adding more servos or changing timer libraries. The brief interrupt masking can make millis-based intervals slightly longer than wall-clock time; they are expressive timing, not precision timing.

After uploading, check: a blue startup animation followed by a sleeping face with floating Zs; increasingly impatient expressions and motion during rapid pokes; a tired expression after sustained activity; visible parameter recovery after two quiet seconds; sleepy/sleeping expressions after a longer rest; immediate wake on the next accepted switch activation; correct orientation; and no servo twitch when the face changes. If the image zigzags, check the serpentine setting. If it is sideways, change rotation.

## Piezo voice

With power disconnected, wire **D8 → 330 ohm resistor → passive piezo +**, and **piezo − → GND**. This is for a small passive piezo transducer, not a low-impedance speaker. A marked + terminal goes toward D8. No additional library is needed.

| Mood | Sound |
| --- | --- |
| Happy | Fast rising sparkle sweeps |
| Curious | Rising/falling questioning chirp |
| Playful | Quick bouncing melody |
| Annoyed | Repeated low grumble |
| Furious | Low staccato pitch dives and rough warbles |
| Tired | Slow descending sigh |
| Sleepy | Gentle descending notes |
| Asleep | Silent; waking interaction uses the new awake mood |

Sounds start only on an accepted switch activation. Startup, idle mood changes, cooldown and sleep are silent. Switching OFF cancels playback after input debouncing; waiting for a stuck switch to turn OFF is silent too. A new interaction replaces any unfinished phrase. Each phrase lasts under half a second and ends with the output LOW. Playback does not block switch sampling or servo movement.

Set SOUND_ENABLED=false in include/config.h to mute. PIEZO_PIN is D8. Edit start/end frequencies, duration and warble depth in include/sound_patterns.h to tune the voice; zero start/end frequencies mean a rest. Sweeps update every 4 ms (250 Hz pitch-control rate, not audio sample rate), with optional 8 ms alternating pitch modulation. Five short syllables and 24 ms gaps create quick robotic chatter. Pitch and rhythm convey emotion; this simple circuit has no software volume control. The actual timbre/loudness depends on the piezo.

Arduino tone() uses Timer2 on this Nano; Servo uses Timer1. Avoid using Timer2 PWM on D3/D11 alongside sound. The grid defers LED transfers during audible notes, updating during short rests and still respecting the servo pulse gap; this prevents the grid's interrupt masking from breaking up a note. A face change may therefore be delayed by up to one syllable (96 ms with these phrases).

After upload, check silence at startup and throughout idle cooldown/sleep. Flip ON for questioning chirps, poke repeatedly for sharper angry warbles, then check that OFF promptly cancels sound. Confirm smooth arm movement and no repeated sound for a held ON.

## Two-servo wiring and calibration

| Device | Nano signal pin |
| --- | --- |
| Switch 1 (closes to GND) | D4 |
| WS2812B grid DIN | D7 |
| Passive piezo | D8 |
| Arm servo | D9 |
| Lid servo | D11 |

The Servo library generates both servo signals with Timer1, even on D11. This does not use D11 hardware PWM, so the Timer2 piezo sound remains compatible. Keep D0/D1 available for USB serial.

Connect all servo positive wires to an external regulated supply matching their voltage rating; all servo grounds, Nano GND, LED ground and supply negative must be common. Size the supply for the two servos' combined stall current plus the LED load if sharing it. Use adequate direct power distribution instead of carrying motor current through Nano pins or thin breadboard rails. Keep the external positive supply separate from the USB-powered Nano 5V pin.

New starting angles in include/config.h:

- LID_CLOSED_DEG = 60; LID_OPEN_DEG = 110.
- LID_STEP_MS = 6 ms per degree.
- LID_SETTLE_MS = 400; STARTUP_ARM_SETTLE_MS = 800.

These are uncalibrated guesses. Remove horns/linkages for the first upload. Check servo direction and usable endpoints, power off before fitting horns, then adjust angles in small increments. Confirm the arm's HOME_DEG clears the lid throughout its travel. Increase settling times if the physical mechanism takes longer than the commands; there is no position feedback or obstruction detection.

Startup opens the lid first, waits 1.6 seconds while the grid intro runs, enables the arm at home, waits 800 ms, then closes the lid and settles. This staged homing assumes the lid can open from any startup pose and the arm can retract from its startup position. Check those clearances mechanically. Startup remains asleep/silent until an accepted switch activation; ON at boot is handled after homing finishes.

Normal sequence: retract/settle if necessary → open lid → wait for lid clearance → mood-specific arm routine → retract → wait for arm clearance → close lid → wait → idle. Accessory speeds are fixed and adjustable; mood continues to control the arm.

After closing and the 400 ms settling period, the lid servo is detached and D11 is driven LOW. This stops continuous position-holding pulses during idle, sleep, and waiting for a held switch to turn OFF. The next opening reattaches it at its last commanded angle. The lid remains driven while open to prevent it falling onto the arm; the closed lid must rest mechanically without servo torque. Detaching stops control pulses, not the servo's power supply, and cannot guarantee zero current on every servo.

If the lid servo draws sustained high current or gets hot, disconnect power and let it cool. Calibrate LID_CLOSED_DEG and LID_OPEN_DEG so neither position drives against a hard stop or strained linkage. The firmware has no current or position feedback and cannot detect a stall during motion. If high current persists while stowed with D11 LOW, test the servo unloaded and check its wiring, rated supply voltage, and condition; pulse release alone cannot fix that.

A new flip can interrupt an accessory operation, but cannot bypass clearance. Turning OFF during preparation cancels the push; the mechanism finishes the necessary stowing sequence. A missed push while ON stays held produces one attempt and then waits with the mechanism closed. Only commanded positions plus time-based settling are known to the software.

Test on the breadboard: normal cycle; OFF while opening; immediate re-flip during arm return; re-flip while the arm settles or lid closes; held ON after a missed push; ON at boot; face updates during two-servo motion. Verify no twitching and no collision before connecting the assembled mechanism.

## References

- [PlatformIO classic Nano](https://docs.platformio.org/en/stable/boards/atmelavr/nanoatmega328.html)
- [PlatformIO new-bootloader Nano](https://docs.platformio.org/en/stable/boards/atmelavr/nanoatmega328new.html)
- [Arduino Nano processor/bootloader selection](https://support.arduino.cc/hc/en-us/articles/4401874304274-Select-the-right-processor-for-Arduino-Nano)
- [Arduino servo troubleshooting and power](https://support.arduino.cc/hc/en-us/articles/360017053760-Troubleshoot-servo-motors)

## Build verification

The current nanoatmega328new environment compiles successfully. Flash: 15,314 bytes; static RAM: 489 bytes. The NeoPixel library additionally allocates a 192-byte pixel buffer on the heap, so the build's static RAM figure does not include all runtime memory. Hardware upload, display orientation and mechanical operation still need verification.

Compile-time scenario simulations in tests/logic_checks.cpp passed for rapid escalation, delayed recovery, full rest and wake-up, low energy overriding anger, independent trait effects, trait recovery, sleep and disturbance, 1,000 activations/rest ticks without parameter overflow, all 16 matrix orientation/layout combinations, sound phrase bounds/rest windows, note timing across millis() rollover, transition completion/retargeting, color-blend endpoints, staged motion profiles, reverse-mounted peek positions, and quick-repeat rearming across timer rollover. The project uses C++14 so the same model can be evaluated during these checks. Run with the PlatformIO AVR compiler:

~~~sh
avr-g++ -mmcu=atmega328p -std=c++14 -Iinclude -c tests/logic_checks.cpp -o .pio/logic_checks.o
~~~

The compiler is under ~/.platformio/packages/toolchain-atmelavr/bin if it is not on PATH. These checks do not exercise physical LED/servo timing.

### Grid mapping correction

The recorded sleep animation confirmed that this panel needs MATRIX_SERPENTINE=false. The former true setting reversed alternate rows, breaking the diagonal and separating the Z bars. No rotation or mirror change was needed. Upload this configuration change to apply the fix.

### Controller simulation

Twenty-two host simulations passed using tests/host/mechanism_test.cpp, which includes the actual src/main.cpp with fake Arduino I/O and servo objects. Each tick checks that arm extension requires a settled open lid, lid closure requires the arm stowed, and the lid servo releases after closure. Scenarios cover normal operation, OFF during lid settling/opening, a quick repeat during arm return, a repeat during arm settling/lid closure, held ON after a miss, and ON at boot. These validate commands and state transitions, not physical clearance or measured servo position.

Run with a host C++14 compiler (for example Zig installed in the ignored .pio/host-tools folder):

~~~powershell
.pio/host-tools/ziglang/zig.exe c++ -target x86_64-windows-gnu -std=c++14 -Itests/host -Iinclude tests/host/mechanism_test.cpp -o .pio/mechanism_test.exe
0..21 | ForEach-Object { & ./.pio/mechanism_test.exe $_; if ($LASTEXITCODE -ne 0) { throw 'Simulation failed' } }
~~~

All active moods bypass the software ramp and immediately command 60° to push; tired returns gradually, while other active moods command 180° directly. Actual speed depends on the servo and load. The 500 ms push timeout and 500 ms home settling period remain, allowing physical movement before the lid closes. The host tests cover direct-command moods, curious/suspicious creep-then-slam and this settling guard.

From fully stowed idle, opening starts without repeating the 500 ms arm-home wait. Reaction and peek pauses are shortened; curious uses 120 ms and suspicious 250 ms. Lid clearance and post-retraction settling remain unchanged. The 60% peek position must stay clear of switch contact.

Playful is the mischievous fake-out: open and settle the lid, command 80° immediately, hold for 700 ms (including servo travel), retract to 180° and settle, close/settle/release the lid, wait another 650 ms, reopen/settle, then command 60° directly. This is one interaction: no second personality activation or sound trigger. OFF cancels the pending slam at any stage. A fresh activation replaces the routine; rapid repeats skip the fake-out to preserve immediate response. Configure MISCHIEF_PEEK_DEG, MISCHIEF_PEEK_PAUSE_MS and MISCHIEF_CLOSED_WAIT_MS in include/config.h. The 80° stop must remain clear of switch contact. Tests include fake-out completion, cancellation while peeking/waiting/reopening, and retries while closing or waiting.

Behavior variants rotate across accepted activations. Happy alternates a direct swat with short 100/200 ms inspecting pauses. Curious uses a 6 ms/degree creep + 120 ms pause, a quicker 3 ms/degree approach + 240 ms pause, or a direct peek + 350 ms stare. Suspicious uses 8/5/10 ms per degree with 250/400/550 ms pauses. Playful always preserves the 80° fake-out, with 700/900/1100 ms peeking and 650/850/1050 ms hiding. Tired keeps its calibrated 100° stop and 900 ms pause. Rapid re-flips still skip teasing. From full anger, five quiet seconds reduce annoyance below the annoyed threshold, and ten clear it completely; fully exhausted energy may still select tiredness.
