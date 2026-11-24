# Useless machine: personality and behavior flowchart

Updated for one arm servo and one switch on 2026-11-24. Moods are recalculated from the parameters, not selected from a fixed sequence. The machine can skip moods.

Source: `include/personality.h`, `include/interaction.h`, `src/main.cpp`, `src/face.cpp`, and `include/sound_patterns.h`.

## 1. Actions change its personality

All parameters stay between **0 and 100**. At startup: energy **90**, annoyance **0**, excitement **0**, curiosity **40**, patience **80**, mischief **25**.

```mermaid
flowchart TD
    INPUT{"What happens?"}
    INPUT -->|"Fresh ON, stable for 30 ms"| ACCEPT["Accept one interaction<br/>Can interrupt an existing arm routine"]
    INPUT -->|"Switch stays ON"| HELD["No repeated activation<br/>No recovery"]
    INPUT -->|"Switch turns OFF"| OFF["Stop sound<br/>Rearm for the next ON<br/>Cancel approach/push and retract"]
    INPUT -->|"Idle at home, switch OFF"| REST["Accumulate quiet time"]
    INPUT -->|"Reset / power on"| RESET["Reset personality<br/>Home arm + 2.4 s intro<br/>Start asleep, silently"]
    ACCEPT --> RAPID{"Previous activation exists<br/>and quiet time below 8 seconds?"}
    RAPID -->|No| SPACED["Spaced interaction<br/>Annoyance +3<br/>Patience −3<br/>Curiosity +22<br/>Excitement +20"]
    RAPID -->|Yes| REPEAT["Repeated poking<br/>Annoyance +10 to +15<br/>Lower patience = larger increase<br/>Patience −10<br/>Curiosity −12<br/>Excitement +24"]
    SPACED --> MISCHIEF{"Updated annoyance below 55?"}
    REPEAT --> MISCHIEF
    MISCHIEF -->|Yes| FUN["Mischief increases<br/>+16 spaced / +8 repeated"]
    MISCHIEF -->|No| UNFUN["Mischief −12"]
    FUN --> COST["Energy −4 spaced / −8 repeated<br/>Play streak +1, or reset to 1 after quiet break<br/>Activation count +1<br/>Quiet timer resets"]
    UNFUN --> COST
    COST --> SELECT["Recalculate mood<br/>Choose arm routine and sound"]
    REST --> SECOND["Every quiet second:<br/>Excitement −4"]
    SECOND --> ANGER["After 2 quiet seconds: annoyance −20 per second"]
    ANGER --> GRACE{"At least 2 quiet seconds?"}
    GRACE -->|No| RECALC["Recalculate mood<br/>Update face; stay silent"]
    GRACE -->|Yes| RECOVER["Each second:<br/>Energy +4<br/>Patience +5<br/>Mischief moves 3 toward 25"]
    RECOVER --> NOVELTY["Curiosity gradually rebuilds toward 70<br/>+2 each recovery second"]
    NOVELTY --> RECALC
```

For repeated poking, the exact annoyance increase is `10 + floor((100 − patience) / 20)`, using patience **before** that poke.

Quiet time starts only once the arm finishes returning and settling. Movement or a held-ON switch prevents recovery. Cancelling an interaction does **not** refund its parameter changes. A switch already ON at startup gets one attempt after startup settling.

Quiet time saturates at 300 seconds. Anger and other recovery begin at 2 seconds; the separate repeated-poking window remains 8 seconds. From total depletion, energy is full by 26 quiet seconds, and curiosity reaches 70 by 36 seconds. Curiosity above 70 is not reduced by recovery.

## 2. Parameters select the mood and response

Read from top to bottom. **The first matching condition wins.**

```mermaid
flowchart TD
    START["Evaluate current parameters"] --> Q1{"Quiet ≥60 seconds<br/>and annoyance below 20?"}
    Q1 -->|Yes| SLEEP["ASLEEP<br/>Closed eyes + breathing + drifting Zs<br/>Silent; arm stays home"]
    Q1 -->|No| Q2{"Energy ≤25?"}
    Q2 -->|Yes| TIRED["TIRED<br/>Droopy face<br/>Slow push and slower retreat<br/>Descending electronic sigh"]
    Q2 -->|No| Q3{"Annoyance ≥75?"}
    Q3 -->|Yes| FURIOUS["FURIOUS<br/>Red stare<br/>Immediate, fastest swat<br/>Low pitch dives and warbles"]
    Q3 -->|No| Q4{"Annoyance ≥55?"}
    Q4 -->|Yes| ANNOYED["ANNOYED<br/>Amber unimpressed face<br/>Immediate fast swat<br/>Short grumbling sounds"]
    Q4 -->|No| Q5{"Quiet ≥30 seconds<br/>and excitement below 20?"}
    Q5 -->|Yes| SLEEPY["SLEEPY<br/>Heavy eyelids<br/>Silent; arm stays home"]
    Q5 -->|No| Q6{"Annoyance ≥15<br/>and curiosity ≥45<br/>and excitement ≥10?"}
    Q6 -->|Yes| SUSPICIOUS["SUSPICIOUS<br/>Purple side-eye<br/>Creep out → watch → push<br/>Questioning chirps"]
    Q6 -->|No| Q7{"Play streak ≥3<br/>Excitement ≥60<br/>and mischief ≥35?"}
    Q7 -->|Yes| PLAYFUL["PLAYFUL<br/>Lavender wink<br/>Peek → tease → dart forward<br/>Bouncy robotic chatter"]
    Q7 -->|No| Q8{"Curiosity ≥45<br/>and excitement ≥10?"}
    Q8 -->|Yes| CURIOUS["CURIOUS<br/>Blue questioning face<br/>Cautious approach → brief inspection → push<br/>Rising and falling chirps"]
    Q8 -->|No| HAPPY["HAPPY<br/>Mint smile<br/>Short pause → smooth push<br/>Bright rising chirps"]
```

Sounds happen **only on an accepted activation**. Idle mood changes remain silent. When waking, the activation changes the parameters and resets quiet time **before** selecting the response; it therefore uses a new awake mood.

### Arm timings

| Mood | Routine |
|---|---|
| Happy | Bob out to 160°, hold 250 ms, home, hold 250 ms, swat |
| Curious | Inspect at 108°, creep to 90°, inspect 300 ms, slam |
| Suspicious | Approach 108°, hesitate, withdraw to 150°, wait 400 ms, lunge |
| Playful | Dart to 80°, pause, retract to 180°, settle, hide, slam |
| Annoyed | Immediate single swat |
| Furious | Swat, then only if still ON after 500 ms: recoil to 108°, wait 300 ms, second swat; stop after two |
| Tired | Creep at 18 ms/degree to 100°, wait 900 ms, direct slam to 60°, slow return |

Curious first approach/pause variants: 6/3/0 ms per degree and 120/240/350 ms. Suspicious: 8/5/10 ms per degree and 250/400/550 ms. Playful peek waits: 700/900/1100 ms; hidden waits: 650/850/1050 ms. Variant and routine are frozen per activation. Zero movement interval means a direct command, not instant physical arrival. All angles shown assume home 180° and push 60°.

## 3. Motion and interruption flow

```mermaid
flowchart TD
    ON[Accepted ON: update personality and play sound once] --> LEAD[Hold current arm target for 400 ms]
    LEAD --> VARIANT{Per-mood variant 2 or 3?}
    PUSH[Direct push to 60 degrees]
    VARIANT -->|No| MODE{Selected routine}
    VARIANT -->|Yes| SCRIPT[Play bounded gesture sequence; each step moves then pauses]
    SCRIPT --> TIRED{Tired routine?}
    TIRED -->|No| PUSH
    TIRED -->|Yes| RESET
    MODE -->|Happy| BOB[Bob to 160; pause; home; pause]
    BOB --> PUSH
    MODE -->|Curious| LOOK[Inspect at 108; then at 90]
    LOOK --> PUSH
    MODE -->|Suspicious| DOUBT[Approach 108; hesitate; retreat 150; hesitate]
    DOUBT --> PUSH
    MODE -->|Playful| TEASE[Dart to 80; pause; return home]
    TEASE --> HIDE[Settle 500 ms; hide 650 to 1050 ms]
    HIDE --> PUSH
    MODE -->|Tired| RESET{Already at or beyond 100 degree peek?}
    RESET -->|Yes| HOME[Return home and settle]
    RESET -->|No| CREEP[Creep to 100 at 18 ms per degree]
    HOME --> CREEP
    CREEP --> PAUSE[Pause 900 ms]
    PAUSE --> PUSH
    MODE -->|Annoyed or Furious| PUSH
    PUSH --> HOLD[Hold up to 500 ms]
    HOLD --> AGAIN{Furious, still ON, and only one jab?}
    AGAIN -->|Yes| RECOIL[Recoil to 108; wait 300 ms]
    RECOIL --> PUSH
    AGAIN -->|No| BACK[Retract to 180 and settle 500 ms]
    BACK --> WAIT{Still ON?}
    WAIT -->|Yes| BLOCK[Wait for OFF; no repeated routine]
    WAIT -->|No| IDLE[Idle and recover]
    BLOCK -->|OFF| IDLE
    OFF[Debounced OFF during any gesture or pause] --> BACK
    NEW[Fresh ON replaces any unfinished routine] --> ON
```

The 30 ms debounce applies to every switch change. OFF cancels a pending fake-out or second jab. A fresh activation replaces old pending actions; it never queues an old surprise push. Quick repeats run the full selected variation after the sound lead-in, including tired creep/pause. A tired repeat starting at or beyond its peek resets home before approaching again. The furious second jab belongs to the same activation and is skipped as soon as OFF is detected.

## 4. Hardware and background activity

One servo only: D9 arm, D4 switch, D7 grid, D8 passive piezo. D11 is unused; there is no lid/pivot sequencing or clearance wait. The arm itself must clear the physical box throughout all gestures. Startup commands 180° during the 2.4-second silent face intro, then enables interactions. ON at boot gets one routine; OFF at boot remains asleep at rest.

The face keeps its mood transitions, blinking, breathing and a continuous stream of two 4×4 Zs. The letters move upward every 160 ms, staggered by five frames, and fully exit the top before restarting; rows 5–7 remain reserved for the face. Sound occurs only on accepted interactions; no sound on recovery/sleep or extra gesture stages. Timer1 drives one Servo object. LED transfers start at 4–14 ms of its 20 ms frame and require D9 LOW. The arm remains attached at home.

Sleepy/asleep motion profiles exist, but ordinary activation clears quiet time before routine selection, so their faces occur during rest while their motion profiles are normally unreachable. Tests cover active moods, cancellation, quick repeats including tired, bounded furious retries, startup, debounce and millis rollover.

Every accepted activation, including quick repeats, waits RESPONSE_PAUSE_MS = 400 ms after starting sound before dispatching its routine. OFF cancels this pending response and allows immediate retraction.

## 5. Three choreographies per active mood

Each routine keeps its own counter modulo three, advanced on accepted activation, starting with expressive variant 2 then rotating 3 → 1 → 2. Variant one follows the graph's original mood path; variants two and three follow a flash-stored sequence, then perform the direct push (or, for tired, continue through its slow 100° approach and 900 ms pause first). ResponsePause always happens first. Quick repeats no longer bypass theatre.

| Mood | Variant 2 | Variant 3 |
|---|---|---|
| Happy | Double greeting bob | Bow, wind back, swat |
| Curious | Double take with two retreats | Three-step approach, think, back up, inspect |
| Suspicious | Startle, fully hide, cautious return | Two aborted approaches, disappear, lunge |
| Playful | Double 80° fake-out and tiny peek | Long hide, two peeks, surprise attack |
| Annoyed | Two non-contact air-knocks | Freeze near switch, withdraw, swat |
| Furious | Wind-up shake | Charge, brake, recoil, attack |
| Tired | Nod off twice | Two droopy false starts |

ScriptMove advances nonblocking toward one target. ScriptPause waits its bounded duration before loading the next step. OFF cancels either state; fresh ON replaces the entire remaining sequence after a new 400 ms sound lead-in. Scripted gestures cannot trigger their own personality updates or sounds. Furious's second-jab rule and all rest endpoints remain unchanged.

## Mood progression tuning

Playful requires three pokes in a streak and excitement at least 60. A poke after eight quiet seconds starts a new streak at one; patience, curiosity, energy and anger still affect which mood wins. Spaced pokes cost 4 energy, repeated pokes cost 8. From startup with no recovery, playful first appears on poke three, annoyed on six, furious on seven/eight, and tired on nine. Tired takes priority over furious; fury remains possible when annoyance is high and energy sufficient. Maximum annoyance falls below the annoyed threshold after four idle/OFF seconds and clears after six. All cooldown starts after two quiet seconds. Movement or held ON prevents recovery.
