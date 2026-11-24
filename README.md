# Useless machine

Assembled box using a classic 5 V / 16 MHz ATmega328P Nano clone, **one arm servo**, one maintained switch, an 8×8 WS2812B face and a passive piezo. There is no lid or pivot servo. D11 is unused.

## Wiring and power

| Component | Nano connection |
|---|---|
| Switch | D4 to switch to GND; INPUT_PULLUP, LOW = ON |
| Arm servo signal | D9 |
| WS2812B DIN | D7 through a 330–470 ohm resistor |
| Passive piezo | D8 through 330 ohms to +; other lead to GND |
| Onboard L LED | D13 held LOW while firmware runs |

Use an external regulated 5 V supply appropriate for the servo, with enough current for its stall current plus the LED grid. Feed the servo/grid directly from the supply rails; don't route motor current through the Nano's USB/regulator. Connect every ground together. Use short, adequate copper power distribution on perfboard. Add 500–1000 µF across the grid supply near its connector, correct polarity and voltage rating above 5 V. Capacitors don't replace an adequate supply.

For USB programming, keep the external positive rail separate from the USB-powered Nano's 5V pin unless power isolation is verified. A standalone regulated 5 V rail can feed the classic Nano's 5V pin; it must not also be connected to an unverified USB power source. Do not feed regulated 5 V through VIN. D5, D6, D10 and D11 are unused. The power LED and bootloader LED flashes cannot be disabled by this sketch.

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

## Calibration and startup

Current arm endpoints in include/config.h: **HOME_DEG = 180°**, **PUSH_DEG = 60°**. The actual switch push is a direct target command in every active mood; this requests available servo speed, not extra motor torque. Gesture positions must remain clear of switch contact and mechanical stops. Firmware knows commanded positions, not actual shaft position or current.

Startup immediately commands home during the silent 2.4-second face intro. Interaction begins after that settling period. The personality starts asleep. An ON switch at boot gets one routine after startup; an OFF switch leaves the arm resting. The arm remains attached at home. Verify travel without a connected linkage before changing calibration, and disconnect power if the mechanism stalls against a stop.

Every accepted switch activation starts its sound immediately, then holds the current arm target for **400 ms** before responding (RESPONSE_PAUSE_MS). This also applies to rapid re-flips and angry modes. OFF during this pause cancels the response; retraction is not delayed. Existing mood-specific gestures and pauses follow the lead-in.

## Distinct mood routines

| Mood | Movement sequence |
|---|---|
| Happy | Friendly bob: 180° → 160° → 180°, with 250 ms at each stop, then a fast push |
| Curious | Approach 108°, inspect; creep closer to 90°, inspect for 300 ms; then slam |
| Suspicious | Creep to 108°, hesitate; withdraw to 150°, wait 400 ms; then lunge |
| Playful / mischievous | Dart to 80°, pause, fully retract and settle, hide briefly, then slam; no lid movement |
| Annoyed | Single swat after the sound lead-in and return |
| Furious | Swat after the sound lead-in; if still ON after 500 ms, recoil to 108°, wait 300 ms, jab once more; then return |
| Tired | **Slow 18 ms/degree creep to 100°**, pause **900 ms**, slam the final 40° to 60°, then return slowly |

Every successful switch-OFF cancels remaining motion and returns home after 30 ms debounce. Furious never performs its second jab after OFF; a failed routine has at most two jabs and then waits for OFF. Other moods get one actual push per activation. Each push has a 500 ms timeout; return has 500 ms settling. Direct commands need physical travel time; verify these waits on the mechanism.

A fresh OFF→ON replaces the current routine. Rapid re-flips run the newly selected full variation after the sound lead-in; they no longer skip the gestures. **Tired always preserves its slow approach and 900 ms pause**, including rapid repeats. If interrupted at or beyond its 100° peek, it first returns home and settles before creeping again. All motion is nonblocking; switch sampling, sound and face animations keep running.

Curious approaches vary between 6/3/0 ms per degree with 120/240/350 ms first pauses. Suspicious uses 8/5/10 ms per degree and 250/400/550 ms first pauses. Playful keeps its 80° fake-out, varying peek waits between 700/900/1100 ms and hiding between 650/850/1050 ms. These variants rotate per activation; they do not replace the mood's distinctive sequence. Sleepy/asleep faces occur during quiet rest, but their motion profiles are normally unreachable because an activation wakes the personality first.

## Three routines per mood

Each mood has its own three-entry rotation, so coming back to that mood advances to a different sequence. Each mood starts with expressive variant 2, then rotates 3 → 1 → 2. The serial monitor prints Motion variant=1/2/3. Variant 1 uses the familiar sequence above; variants 2 and 3 use the flash-stored choreography in include/choreography.h:

| Mood | Variant 2 | Variant 3 |
|---|---|---|
| Happy | Two enthusiastic greeting bobs, each a different height | Bow forward, wind back, then release into a swat |
| Curious | Double take: approach, pull back, look closer, pull back again | Approach in three steps, think, back up, inspect once more |
| Suspicious | Startle at the switch, hide fully, cautiously reappear | Abort two approaches, disappear, return for a surprise lunge |
| Playful | Two full 80° fake-outs, then a tiny peek before the real attack | Disappear for a long beat, peek out twice, then spring from hiding |
| Annoyed | Two impatient air-knocks short of contact, then swat | Freeze near the switch, pull back, then swat |
| Furious | Short wind-up shake before the bounded attack | Charge toward the switch, brake, recoil fully, attack |
| Tired | Nod off twice and restart | Two droopy false starts before the final slow approach |

These are movement sequences, not just randomized delays. All preliminary gestures stay short of the push endpoint; playful's near-switch stop remains exactly 80°. Tired variants always continue with the slow creep to 100°, the 900 ms pause, and the direct 60° push. Every furious variant still allows at most two actual pushes, and a successful OFF cancels the second. Some gestures deliberately hide for up to 1.4 seconds. Tests cannot guarantee physical non-contact at an intermediate angle: check the assembled mechanism.

The 400 ms sound lead-in is always applied. Quick re-flips retain the full selected variation, including tired’s slow approach. OFF cancels any script immediately after debounce. No extra sounds or personality activations are generated by individual gestures. Only one step is stored in RAM; the 14 new scripts live in flash.

## Personality and recovery

| Parameter | Changes |
|---|---|
| Energy | Starts 90; spaced activation costs 4; repeated activation costs 8; quiet recovery adds 4/second; ≤25 selects tired |
| Annoyance | Spaced poke +3; repeated poke +10…15 depending on patience; recovery −20/second starting after two quiet seconds |
| Excitement | Spaced poke +20, repeated poke +24; quiet −4/second |
| Curiosity | Starts 40; spaced +22, repeated −12; recovery +2/second toward 70 |
| Patience | Starts 80; spaced −3, repeated −10; recovery +5/second |
| Mischief | Starts 25; enjoyable pokes +8…16, irritating pokes −12; recovery moves 3/second toward 25 |

Repeated poking means another activation before eight seconds of idle rest. Anger cooldown starts after **2 seconds continuously idle and OFF**; other recovery starts after 2 seconds. Annoyed begins at 55 annoyance; furious at 75, unless exhausted. A fresh model with no recovery reaches annoyed around poke six, furious around poke seven, and tired around poke nine (previously seventeen). Fatigue takes priority over fury. Four quiet seconds take maximum annoyance below 55, and six clear it. Energy recovers from zero by 26 quiet seconds; curiosity from zero by 36. Holding ON or moving prevents recovery.

Playful now requires at least **three pokes in a play streak**, excitement ≥60 and mischief ≥35. Eight quiet seconds reset that warm-up on the next activation, so high leftover excitement cannot jump straight back into playful. Other mood priorities still apply.

Calm machines become sleepy after 30 quiet seconds and asleep after 60. Sleep does not switch off servo power. Startup resets personality; no EEPROM storage. The serial monitor at 115200 baud prints mood and all six parameters on activation, mood changes and every five seconds. See docs/personality-flowchart.md for the decision graph.

## Face and sound

The face uses nine expressions and animated eye-close / blend / eye-open transitions, plus blinking and breathing. Asleep shows two staggered 4×4 Zs in separate lanes above the face. They advance upward every 160 ms (previously 220 ms), clip naturally at the top, fully exit, and reappear above the eyes in a continuous stream. Startup fills the whole 8×8 panel with a diagonal rainbow ignition and moving color ripples, then dissolves into the sleeping face over 2.4 seconds (60 frames at 40 ms). The effect uses the existing brightness limit and servo-safe update window. The panel uses straight rows (MATRIX_SERPENTINE=false); rotation and mirroring are configurable. Current FACE_BRIGHTNESS is 10/255. SLEEP_Z_STATIC_TEST freezes the Z for orientation checks.

WS2812B transmission masks interrupts for about 1.92 ms. With **one Servo 1.2.2 object on a 16 MHz ATmega328P**, transfers start at **4–14 ms** of the 20 ms timer frame, only while D9 is LOW. Revisit this timing if adding Servo objects or changing timer libraries. Timer1 controls the servo; Arduino tone uses Timer2. Face updates avoid audible tones to protect both sound and servo pulses.

The passive piezo uses mood-specific phrases with 4 ms pitch updates. Sound starts only on accepted interaction and stops on OFF; startup, sleep and recovery are silent. Multi-stage routines do not create additional activations or sounds. SOUND_ENABLED=false mutes the project.

## Verification

Firmware builds for nanoatmega328new. Static RAM and flash figures are reported by PlatformIO; the NeoPixel library also allocates a 192-byte pixel buffer on the heap. No automatic hardware upload or physical clearance verification is performed.

Compile-only personality, recovery, sound, matrix and animation checks:

~~~sh
avr-g++ -mmcu=atmega328p -std=c++14 -Iinclude -c tests/logic_checks.cpp -o .pio/logic_checks.o
~~~

Sixty-three host simulations include the actual controller with fake Arduino I/O and exactly one servo. They exercise each mood's distinct gesture path, cancellation during pauses, held-ON limits, tired quick repeats from before/after the peek, normal rapid retries, fake-out timing variants, switch debounce, startup-held ON, silent sleep and millis rollover.

~~~powershell
.pio/host-tools/ziglang/zig.exe c++ -target x86_64-windows-gnu -std=c++14 -Itests/host -Iinclude tests/host/mechanism_test.cpp -o .pio/mechanism_test.exe
0..68 | ForEach-Object { & ./.pio/mechanism_test.exe $_; if ($LASTEXITCODE -ne 0) { throw 'Simulation failed' } }
~~~

On hardware, confirm the happy bob, both curious inspection stops, suspicious retreat, playful hiding, tired slow approach (including re-flips), and furious cancellation after the first successful push. Monitor supply voltage at the servo under load and check for mechanical interference. Software simulations cannot measure shaft position, torque or current.

## References

- [Arduino servo power and basics](https://docs.arduino.cc/learn/electronics/servo-motors/)
- [Adafruit NeoPixel wiring guidance](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices)
- [PlatformIO classic Nano](https://docs.platformio.org/en/stable/boards/atmelavr/nanoatmega328.html)
- [PlatformIO new-bootloader Nano](https://docs.platformio.org/en/stable/boards/atmelavr/nanoatmega328new.html)

## Interaction tricks and dreams

These are temporary performances layered over the existing mood parameters:

- **I saw that coming:** every third consecutive quick re-flip (ON within 1.2 seconds of OFF) earns a knowing wink, a fast move to 80�, a 550 ms stare, then a direct push to 60�. Tired keeps its usual creep instead. A slower re-flip resets the streak.
- **Fake sleep:** every second playful encounter pretends to sleep at 180� for 1.3 seconds, opens one eye for 650 ms, then slams. Anticipation takes precedence if both qualify. Normal playful encounters retain their rotating fake-outs.
- **Victory:** only the first three OFF edges detected while pushing after each power-up qualify. After retracting, the display celebrates for 1.4 seconds: wink/confetti, pulsing heart, then colorful border confetti. A new ON immediately interrupts it. An aborted approach or a failed push never counts. Celebrations are silent and keep the arm home.
- **Dreaming:** after 12 seconds continuously asleep, a dim thought picture fades in and out for 3.2 seconds, repeating every 18 seconds. Heart, star, and switch pictures cycle above the sleeping face. Normal moving Zs return between dreams. Dreams are silent and cause no arm motion.

All responses retain the 400 ms sound lead-in, switch debouncing, OFF cancellation, and calibrated 180� rest / 60� push. No new mood parameters or extra idle sounds are introduced.
