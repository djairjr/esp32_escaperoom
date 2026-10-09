# H4CK-OS — Development Plan (v3)

## 1. Overview

### Concept
The M5Stack Cardputer poses as a **tiny hacker computer** hidden in the room.
The player finds the device, logs in, and uses simulated pentest tools to
defuse the bomb (BombTimer StamPLC).

### Operating Cycle

```
[POWER ON]
    │
    ▼
[BOOT SCREEN]   ← H4CK-OS-style boot animation (2-3s)
    │
    ▼
[LOGIN SCREEN]  ← login: root / password: (found in the room)
    │
    ▼
[SHELL]         ← bash-like with commands
    │
    ├── scan        — discover devices on the network
    ├── tapscan     — intercept phone conversations (MP3 from SD)
    ├── nmap        — detailed scan of a target
    ├── hydra       — password brute force
    ├── sniff       — capture the bomb code via IP (clue in the room)
    ├── exploit     — generate RF433 payload
    ├── rfcomm      — transmit a real RF433 command
    ├── ha          — control the room devices
    ├── status      — show the bomb status
    ├── clear       — clear the terminal
    └── logout      — return to login
```

---

## 2. Hardware

| Component | Function | Interface |
|------------|--------|-----------|
| **M5Stack Cardputer** | Hacker terminal | ESP32-S3, TFT 240×135, 56-key QWERTY keyboard, NS4168 I2S speaker |
| **SYN531R RF433 TX** on Grove | Transmission of the defuse command | GPIO G2 (DATA) |
| **microSD Card** | MP3 audio (conversations) + assets | SPI (G12 CS, G14 MOSI, G40 CLK, G39 MISO) |

### Cardputer Pinout

```
PORT.CUSTOM (Grove HY2.0-4P):
  Yellow = G2  → RF433 DATA (SYN531R Transmitter)
  White  = G1  → free (reserve)
  Red    = 5V → VCC SYN531R
  Black  = GND → GND SYN531R

SD Card: G12(CS) G14(MOSI) G40(CLK) G39(MISO)
Speaker: NS4168 I2S (G41 BCLK, G42 DATA, G44 LRCLK)
Display: ST7789V2 SPI (G38 BL, G33 RST, G34 RS, G35 DAT, G36 SCK, G37 CS)
Keyboard: 4×14 matrix via TCA8418 (internal I2C)
```

---

## 3. Software Architecture

### 3.1 File Structure

```
~/shared_files/Code/ESP32_Projects/Cardputer_H4CK-OS/
├── H4CK-OS.ino                  ← setup() + loop()
├── src/
│   ├── Shell.h / .cpp           ← command parser + dispatch
│   ├── BootScreen.h             ← H4CK-OS boot animation
│   ├── LoginScreen.h            ← login + password
│   ├── Scanner.h                ← scan + nmap (simulated)
│   ├── TapScanner.h             ← tapscan: MP3 from SD (EscapeRoomPhone pattern)
│   ├── Cracker.h                ← hydra (simulated brute force)
│   ├── Sniffer.h                ← sniff: captures the code configured by the GM
│   ├── Exploit.h                ← generates RF payload
│   ├── Transmitter.h            ← rfcomm via real RF433 (rc-switch)
│   ├── BombStatus.h             ← bomb status via WiFi
│   ├── HAClient.h               ← ha + HA entities for the Cardputer itself
│   ├── SoundManager.h           ← beeps + playOnceBlocking
│   ├── ConfigManager.h          ← JSON config in SPIFFS/LittleFS
│   └── Config.h                 ← pins, constants
├── data/
│   ├── taps/
│   │   ├── channel1.mp3         ← intercepted conversation 1
│   │   ├── channel2.mp3         ← intercepted conversation 2
│   │   ├── channel3.mp3         ← intercepted conversation 3
│   │   └── channel4.mp3         ← intercepted conversation 4
│   ├── scan.mp3                 ← scan sound
│   ├── exploit.mp3              ← payload generated sound
│   ├── transmit.mp3             ← RF transmission sound
│   ├── success.mp3              ← puzzle solved
│   └── error.mp3                ← error / access denied
├── config.json                  ← default configuration (SPIFFS)
├── docs/
│   ├── ALGORITMO.MD             ← this document
│   └── USER_MANUAL.MD           ← player manual (future)
└── ALGORITMO.MD                 ← copy at the root
```

### 3.2 State Machine

```
[POWER ON] → [BOOT] (3s) → [LOGIN] → [SHELL]
                                        │
            ┌──────────┬──────────┬─────┼─────┬──────────┬──────────┐
            ▼          ▼          ▼     ▼     ▼          ▼          ▼
       [STAGE A]  [STAGE B]  [STAGE C] [HA] [SNIFF]  [TAPSCAN]  [STATUS]
       scan/nmap    hydra     exploit   cmd   code     MP3 SD    bomb
```

---

## 4. Home Assistant Integration — Cardputer Entities

**The player does NOT access MQTT directly.** The `sniff` uses a pre-configured IP
(clue in the room) and simulated data — it is not a real MQTT connection.

**But the Cardputer itself exposes entities in Home Assistant** so the Game Master
can configure the puzzle remotely:

### Cardputer HA Entities (for the GM)

| Type | ID | Description | Use |
|------|----|-----------|-----|
| **Text** | `h4ckos_login_password` | Login password | GM sets the password |
| **Text** | `h4ckos_sniff_ip` | Target IP for sniff | GM sets which IP the player needs to "invade" |
| **Number** | `h4ckos_defuse_code` | Defuse code | GM sets the code that sniff reveals |
| **Select** | `h4ckos_sniff_channel` | Sniff audio channel | GM switches which MP3 plays (1-4) |
| **Select** | `h4ckos_stage` | Current stage | GM advances/resets stages manually |
| **Sensor** | `h4ckos_status` | Terminal status | "login", "shell", "defused" |
| **Switch** | `h4ckos_rf433_enable` | Enables RF433 | GM allows/blocks RF transmission |

### Example configuration:

```
GM in HA:
  h4ckos_login_password = "kali"
  h4ckos_sniff_ip = "10.0.0.5"
  h4ckos_defuse_code = "7391"
  h4ckos_sniff_channel = "2"
  → player discovers IP "10.0.0.5" in the room
  → sniff reveals code "7391"
  → can defuse via the bomb keypad
```

### Communication: ArduinoHA (same as BombTimer)

```cpp
// HAClient.h
#include <ArduinoHA.h>

HAText haLoginPassword("h4ckos_login_password");
HAText haSniffIp("h4ckos_sniff_ip");
HANumber haDefuseCode("h4ckos_defuse_code");
HASelect haStage("h4ckos_stage");
HASensor haStatus("h4ckos_status");
HASwitch haRFEnable("h4ckos_rf433_enable");

// Callbacks save to the SPIFFS JSON
void haLoginPasswordCb(const char* val, HAText*) { /* saves to config.json */ }
void haSniffIpCb(const char* val, HAText*)       { /* saves to config.json */ }
```

---

## 5. Configuration via SPIFFS (config.json)

### 5.1 Format

```json
{
  "login": {
    "user": "root",
    "password": "kali"
  },
  "sniff": {
    "ip": "10.0.0.5",
    "defuse_code": 7391,
    "channel": 2
  },
  "stages": {
    "current": 0,
    "scan_enabled": true,
    "hydra_enabled": false,
    "exploit_enabled": false,
    "rf433_enabled": false
  },
  "wordlist": [
    "kali",
    "toor",
    "root",
    "h4ck3r",
    "bomb",
    "defuse"
  ],
  "ha": {
    "broker": "192.168.1.100",
    "port": 1883,
    "user": "",
    "pass": ""
  }
}
```

### 5.2 Configuration Cycle

```
Boot → SPIFFS.begin() → load /config.json
                          │
                    ┌─────┴─────┐
                    │           │
               Exists?    Doesn't exist?
                    │           │
                    ▼           ▼
               Loads       Creates with defaults
               values      Saves to SPIFFS
                    │           │
                    └─────┬─────┘
                          ▼
                  ConfigHA.begin()
                  (WiFi + MQTT + HA entities)
                          │
                          ▼
                  BOOT SCREEN → LOGIN
```

### 5.3 GM changes via HA → SPIFFS

```
GM changes "h4ckos_login_password" in HA
    → callback saves to config.json
    → the next time the player logs out,
      the new password is already in effect
```

---

## 6. Commands in Detail

### 6.1 `tapscan` — Call Interceptor (MP3 from SD)

```
root@h4ck-os:~$ tapscan
[HACK] Scanning cellular frequencies...
[HACK] 4 active channels found.

Channel 1: [BUSY]  — "the defuse code is 7391..."
Channel 2: [IDLE]  — no traffic
Channel 3: [BUSY]  — "the system password is kali"
Channel 4: [IDLE]  — no traffic

root@h4ck-os:~$ tapscan listen 1
[HACK] Intercepting channel 1...
▶ Playing: /sd/taps/channel1.mp3
```

**Implementation:** identical to EscapeRoomPhone:
- `tapscan` → lists channels with status (BUSY/IDLE) based on config.json
- `tapscan listen <n>` → `hw.sound().playOnceBlocking("/sd/taps/channelN.mp3")`
- GM configures which channel is active via HA (`h4ckos_sniff_channel`)

### 6.2 `sniff` — Captures the Code (no real MQTT)

```
root@h4ck-os:~$ sniff 10.0.0.5
[HACK] Connecting to 10.0.0.5:8080...
[HACK] Intercepted bomb configuration:
  → defuse_code: 7391
  → countdown: 180s
  → mode: CODE

root@h4ck-os:~$ sniff code
[HACK] Current defuse code: 7391
```

**Implementation:** `sniff` does NOT make a real MQTT connection.
It simply reads `defuse_code` from `config.json`, which was
configured by the GM via HA. The IP is a clue in the room.

**But it looks real:** the command shows progress, connection,
"intercepted packets", etc.

### 6.3 `scan` / `nmap` — Stage A (simulated)

```
root@h4ck-os:~$ scan
[HACK] Scanning 192.168.1.0/24...
[HACK] 3 devices found:
  192.168.1.100  — BombTimer (RF433)
  192.168.1.101  — Unknown
  192.168.1.1    — Router

root@h4ck-os:~$ nmap 192.168.1.100
[HACK] Scanning 192.168.1.100...
  PORT     STATE  SERVICE
  8080/tcp open   RF433_TRANSMITTER
  9090/tcp open   AUTH_SERVICE
  7070/tcp open   ENCRYPTER
```

### 6.4 `hydra` — Stage B (brute force)

```
root@h4ck-os:~$ hydra 192.168.1.100 -P passwords.txt
[HACK] Brute forcing AUTH_SERVICE on 192.168.1.100:9090...
[HACK] Trying password: kali... FAIL
[HACK] Trying password: toor... FAIL
[HACK] Trying password: root... SUCCESS!
[HACK] Key obtained: 0x7A3F
```

### 6.5 `exploit` — Stage C

```
root@h4ck-os:~$ exploit -k 0x7A3F -p 8080
[HACK] Generating exploit payload...
[HACK] Payload: RF433 code 0x89ABCD
[HACK] Use 'rfcomm send 0x89ABCD' to transmit
```

### 6.6 `rfcomm` — Stage D (REAL RF433)

```
root@h4ck-os:~$ rfcomm send 0x89ABCD
[HACK] Transmitting RF433 signal...
[HACK] Code 0x89ABCD sent!
```

**RF433 is real:** SYN531R transmits via G2. BombTimer receives.

### 6.7 `ha` — Room Control

```
root@h4ck-os:~$ ha list
  light.living_room     — Living Room Light
  switch.fan            — Ceiling Fan
  lock.main_door        — Main Door Lock

root@h4ck-os:~$ ha toggle light.living_room
  → light.living_room toggled
```

**Real MQTT:** connects to the HA broker and sends commands.

### 6.8 `status` — Bomb Status

```
root@h4ck-os:~$ status
[STATUS] Connecting to BombTimer...
  State: RUNNING
  Time:  00:47
  Wires: 2 cut
  Mode:  CODE
```

---

## 7. Audio

### 7.1 MP3 from SD (tapscan)

EscapeRoomPhone pattern:
```cpp
// Play MP3 from SD, blocking until done
hw.sound().playOnceBlocking("/sd/taps/channel2.mp3");

// Sound loop in the main loop()
hw.sound().loop();
```

### 7.2 Synthesized Beeps

```cpp
M5Cardputer.Speaker.tone(4000, 50);   // key
M5Cardputer.Speaker.tone(3000, 30);   // command
M5Cardputer.Speaker.tone(800, 150);   // error
```

---

## 8. Multiple Paths for the Player

| Strategy | Tools | Depends on |
|------------|-------------|------------|
| **RF433** | scan → nmap → hydra → exploit → rfcomm | Clues in the room (passwords) |
| **Code via sniff** | sniff <IP> → type on the bomb keypad | IP clue in the room |
| **Audio clue** | tapscan → listen to conversations | Content of the MP3s |
| **Room control** | ha → interact with devices | None (always available) |

---

## 9. Game Master Configuration Flow

```
1. GM accesses Home Assistant
2. Finds the Cardputer entities:
   - h4ckos_login_password → sets password
   - h4ckos_sniff_ip → sets clue IP
   - h4ckos_defuse_code → sets revealed code
   - h4ckos_sniff_channel → sets which audio channel is active
3. Places clues in the room:
   - Post-it with IP: "10.0.0.5"
   - Document with passwords for hydra
   - Pendrive with MP3 files on the SD
4. Turns on the Cardputer
5. Cycle: boot → login → shell → player explores
```

---

## 10. Schedule

| Phase | Description | Hours |
|------|-----------|-------|
| 1 | Project setup + ConfigManager (SPIFFS JSON) | 2 |
| 2 | Boot screen + Login screen | 2 |
| 3 | bash Shell (parser + help + dispatch) | 3 |
| 4 | tapscan (MP3 from SD) | 2 |
| 5 | Stage A: scan/nmap | 2 |
| 6 | Stage B: hydra | 2 |
| 7 | sniff (code from config.json) | 1 |
| 8 | Stage C: exploit | 1 |
| 9 | Stage D: rfcomm (real RF433) | 2 |
| 10 | Cardputer HA entities + ConfigHA | 3 |
| 11 | BombStatus WiFi | 2 |
| 12 | HAClient (ha commands) | 2 |
| 13 | Audio (beeps + MP3 triggers) | 1 |
| 14 | Integration + tests | 3 |
| **Total** | | **~28h** |
