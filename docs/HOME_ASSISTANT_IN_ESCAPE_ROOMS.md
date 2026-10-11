# Home Assistant for Escape Rooms

How we use the Home Assistant integration, and why.

---

## 1. Where this comes from

Since 2017 I have been building automations for escape rooms, at first with the Arduino
platform and a few single board computers. During the pandemic I moved most of my projects to
RP2040 and ESP32 — Arduino prices had jumped without offering any meaningful platform
improvement. Little by little I built up a modular structure for the games and a set of
conventions; with the arrival of LLMs I could finally write the two engines that do the work
today.

### 1.1 The two engines

**`propmaker_escaperoom` — RP2040.** Designed for the
[Adafruit PropMaker Feather RP2040](https://www.adafruit.com/product/5768), which packs into a
single board what a prop needs: a **MAX98357 I2S 3 W** audio amplifier (sound), a three-axis
**LIS3DH** accelerometer with tap detection, a **NeoPixel** port with screw terminals, a servo
header, and an extra pin that works as an input (button) or an output. In this engine the
puzzles talk to each other over an **RS485** bus (`Rs485Bus`, `Rs485Payload`,
`onNetworkPacket`) — a dedicated wire, deterministic, no network.

**`esp32_escaperoom` — ESP32.** It takes PropMaker EscapeRoom as its base (the same engine:
`PuzzleBase` with a state machine, `LogicManager`, the `hw` singleton, `PuzzlePixelView`,
`AudioPlayer`) and adds Wi-Fi, **MQTT** and, finally, **Home Assistant**. Here RS485 steps
aside: the puzzle becomes a network device like any other.

### 1.2 The design both engines share: an I2C-first build

A puzzle needs far more inputs and outputs than a board has pins. Both engines are designed so
that a build is wired **around I2C**, and not around the few free pins: two wires (SDA/SCL)
carry the whole installation — PCF8574/PCF8575 I/O expanders (through IoAbstraction), an
HT16K33 14-segment display, a PCA9685 PWM driver for servos and dimmers, ADS1115 analog
inputs, a PN532 NFC reader, an LIS3DH accelerometer and a PCF8563 real-time clock.
`Hardware::begin()` runs `Wire.begin()` and then a full I2C scan; every `hw.hasX()` flag comes
from that scan, so a sketch adapts to the hardware that is physically there. In short:
**adding a peripheral costs an I2C address, not a GPIO pin** — which is what keeps a puzzle
box buildable once the board has run out of pins. This matters directly for this document
because the same logic applies to the room: what the library does not solve over I2C, it
solves over the network.

---

## 2. What Home Assistant Green is

[Home Assistant Green](https://www.home-assistant.io/green/) is the official Home Assistant
server, designed by Nabu Casa and built in partnership with the Open Home Foundation. It is
the "ready to use" way to run Home Assistant: plug it in, put it on the network, and the
automation system is running — no Raspberry Pi to assemble.

**Hardware (official specification):**

| Item | Value |
|------|-------|
| SoC | Rockchip RK3566, quad-core Arm Cortex-A55 |
| Frequency | 1.8 GHz |
| Storage | 32 GB eMMC |
| Memory | 4 GB LPDDR4X |
| Network | 1 Gigabit Ethernet port |
| USB | 2 × USB 2.0 type A (5 V, up to 2 A combined) |
| Power | 12 V DC, 1 A (5.5 mm × 2.1 mm barrel connector) |
| Consumption | ~1.7 W idle; ~3 W under load |
| Dimensions | 112 × 112 × 32 mm, 340 g |
| Extras | HDMI (diagnostics only), microSD (recovery only), optional CR2032 (RTC) |
| Reference price | US$ 199 / € 179 |
| Model | NC-GREEN-1175 (CE, FCC) |

It runs **Home Assistant OS** natively.

**Compatibility.** Home Assistant talks to practically anything: the
[integrations catalogue](https://www.home-assistant.io/integrations/) holds thousands of
integrations, organised by category — among them **Camera**, **Lock**, **Media Player**,
**Light**, **Cover**, **Siren**, **Climate** and **Switch**. In other words: a camera, audio
equipment and a lock are not special cases; they are first-class categories of the system,
with state and command. For radio devices (Zigbee/Thread), Green gets a dedicated radio with
the [Connect ZBT-2](https://www.home-assistant.io/hardware/); devices that are already on the
network (Wi-Fi, Ethernet, MQTT) connect directly.

**Why this box in a game room.** It is a small, quiet server that is always on; it draws very
little; it has Ethernet (a room can be wired); it runs 100% locally, with no cloud dependency
— which, in a commercial operation, means that **the show does not depend on the internet**;
and 4 GB of RAM with 32 GB of eMMC is more than enough for one room's automations, with
history and backups.

---

## 3. How the integration works in practice

### 3.1 The puzzle is a device of the room

In `esp32_escaperoom` the examples use the **ArduinoHA** library, which wraps MQTT and speaks
Home Assistant's **MQTT discovery** protocol. In practice: the puzzle publishes its own
description and the entities show up in HA by themselves, **with no hand-written YAML per
entity**.

The network identity comes from `ConfigManager`, which reads `/wifi.json` from the file
system: broker, port, MQTT user and password, and the device identity — `device.name`,
`device.room` and `device.puzzle_id` (defaults: `escaperoom-puzzle`, `sala-01`, `puzzle`).
Moving a puzzle to another room is editing that file, not recompiling. The device
`unique_id` is derived from the **MAC** on purpose: renaming the puzzle in HA does not orphan
its entities.

### 3.2 What the puzzle publishes (and what the room can command)

The entity pattern is already established in the library, and the `BombTimer` example is the
complete case — measured in the code: **8 sensors, 1 lock, 4 buttons, 1 select, 2 numbers and
1 switch**.

| Entity (real example) | ArduinoHA type | What it is for |
|-----------------------|----------------|----------------|
| `state`, `time_remaining`, `wires_cut`, `defuse_mode`, `code_attempts`, `brightness`, `volume`, `tick_status` | `HASensor` | Mirror the state machine and the game variables: FSM state, time left, counters, mode, attempts |
| `relay` | `HALock` | The lock or compartment the puzzle releases |
| `start`, `reset`, `detonate`, `defuse` | `HAButton` | **The room commands the game** |
| `mode` | `HASelect` | Puzzle mode selection |
| `countdown`, `defuse_code` | `HANumber` | Time and code, adjustable from outside |
| `tick_sound` | `HASwitch` | Turn a puzzle feature on or off |

State leaves **without polling**: you register `PuzzleBase::setStateChangeCallback()` and the
callback publishes on the FSM transition. Because `setState()` only fires when the state
really changes, one message goes out per transition — no telemetry flood.

Every puzzle's `loop()` must pump MQTT (`haMqtt.loop()`), the same way it pumps audio
(`hw.sound().loop()`); in configuration (AP) mode, `config.handleAP()` as well.

### 3.3 Pitfalls that have already cost time (and are documented)

- `HASensor::setValue()` takes a **string**, not a state object; `HANumber` reads the incoming
  value with `val.toInt32()`. Using the wrong setter compiles and publishes nothing.
- ArduinoHA callbacks are **C function pointers**: there are no lambdas with captures — use a
  global function and static context.
- The callbacks run **inside the MQTT pump**: do not block or call heavy puzzle code there.
- **One broker per room**, with a distinct `device.name` for each puzzle.

The full technical reference is in the sibling `esp32_escaperoom` repository, in
`docs/HOME_ASSISTANT.md`.

---

## 4. Why Home Assistant

An escape room is a distributed system of heterogeneous things: the puzzles, the cameras, the
lighting, the sound, the doors — usually from different makers, each with its own app. Without
a common layer, every installation becomes a patchwork of parallel controls, and the operator
has to know which app to look at for each part of the room.

The integration exists for one reason, and it is the same one written in the library's README:
**to bring the game automation (puzzle state, locks, timers, sequences) and the room
automation (cameras, lights, sound, doors) under a single management.** The puzzle joins the
room as one more device; the room can drive the game and the game can drive the room.

This was not an afterthought: it is part of the design the engines were born with. The
principles I fixed before writing them are these — and these are the ones the code
implements:

- the game logic runs **locally, in the puzzle**, not on a server;
- room integration is over **MQTT**, and Home Assistant centralises it;
- the language is **operational, not electronic**: door, music, light, time — not pin and
  relay;
- **automations of any smart device** can serve the room's narrative, not only the puzzles.

What this solves, concretely:

1. **A single place.** The same dashboard shows the state of every puzzle, the time left, the
   counters, and next to them the cameras, the lights and the sound. There is no second
   control system in between.
2. **Real operation.** Restarting a puzzle, adjusting the time, unlocking a door or forcing an
   event is one action in the HA app — from a phone, during the game, with no recompiling and
   no separate console. And it is recorded: the Logbook shows what happened and when.
3. **The marginal cost of a new puzzle is one board and one file.** There is no integration to
   write: the puzzle announces itself (discovery). That keeps the library scalable to rooms
   with many puzzles.
4. **The house joins the game.** Any smart device joins the narrative through the same layer —
   the lamp the game turns off, the soundtrack that changes layer, the camera that records the
   reaction — instead of a dedicated special effect for each idea.
5. **Local and predictable.** The server sits in the room, MQTT on the LAN: the game depends on
   neither the internet nor a third-party service.

**Why MQTT:** it is what Home Assistant speaks natively (discovery), it runs on the local
network, it is pub/sub — the puzzle publishes state, the room publishes commands — and it is
exactly what ArduinoHA already implements. The puzzle does not need to "know" what Home
Assistant is; it publishes and listens.

**Why the RP2040 does not use this:** that engine has no network stack; integration between
puzzles is the RS485 bus. The bridge to Home Assistant and to the rest of the room is the
ESP32 sibling, which is the one with Wi-Fi and MQTT. The two live together: the same game, two
transports.

---

## 5. Customizing: how rooms are organized in Home Assistant

In Home Assistant, organisation has three distinct tools, and it is worth using the right
names ([documentation](https://www.home-assistant.io/docs/organizing/)):

- **Area** — a logical grouping of devices and entities that represents a **room or space**
  (living room, kitchen, garage). It is where devices and entities live; a device is **not**
  assigned to a floor. ([areas](https://www.home-assistant.io/docs/organizing/areas/))
- **Floor** — a logical grouping of **areas**, with a **name** and a **level** (a number,
  which can be negative) used for sorting. Floors can be targets of automations — "turn off
  all the lights on the lower floor". ([floors](https://www.home-assistant.io/docs/organizing/floors/))
- **Label** — grouping by any criterion, **independent of physical location**; it applies to
  areas, devices, entities, automations, scenes, scripts and helpers, and works both as an
  automation target and as a filter in tables.
  ([labels](https://www.home-assistant.io/docs/organizing/labels/))

I change little in Home Assistant's original configuration — and the method I found to
organise the rooms uses exactly the **floors**:

> **one floor = one escape room; the areas on that floor = the spaces inside that room.**

The floor level (1, 2, 3…) only sorts the rooms in the interface; what matters is the name,
which is the name of the game. That way a room can still have several spaces, and several
rooms live side by side in the same installation without interfering with each other — each
one on its own floor.

Example:

    Floor: "Room — The Kobold's Talisman"   (level 1)
      ├── Area: Magic Library
      ├── Area: Potion Laboratory
      ├── Area: Hall of Portraits
      ├── Area: Secret Passage
      └── Area: Exit

Why this works well in practice: assigning the area to the discovered device puts its entities
in the right space; an automation can act on **one space** (area) or on **the whole room**
(floor); and everything that is already smart in the room fits into the same scheme, with no
parallel system.

**Where the library cooperates with this:** every puzzle carries `device.room` in `/wifi.json`
(default `sala-01`) and announces itself with that identity. Keeping that field aligned with
the name of the floor/room is what keeps the dashboard readable — the puzzle already knows
which room it is in; all that is left is to assign the area in Home Assistant (Settings →
Areas, labels & zones).

And the **label** comes in as an extra layer, when it helps: for example tagging with a
`puzzle` label every entity that is a puzzle, or `room-talisman` to filter everything in that
room — useful in automations that need "all the puzzles", or in maintenance.

---

## 6. Links

- Home Assistant Green (official): https://www.home-assistant.io/green/
- Official hardware (Green, Yellow, Connect): https://www.home-assistant.io/hardware/
- Integrations catalogue (cameras, locks, media players, lights): https://www.home-assistant.io/integrations/
- Organizing in HA — overview: https://www.home-assistant.io/docs/organizing/
- Areas: https://www.home-assistant.io/docs/organizing/areas/
- Floors: https://www.home-assistant.io/docs/organizing/floors/
- Labels: https://www.home-assistant.io/docs/organizing/labels/
- Adafruit PropMaker Feather RP2040 board: https://www.adafruit.com/product/5768
- `propmaker_escaperoom` (RP2040): https://github.com/djairjr/propmaker_escaperoom
- `esp32_escaperoom` (ESP32, MQTT/Home Assistant): https://github.com/djairjr/esp32_escaperoom
- Telephone dial reader (`RotaryDialUnified`): https://github.com/djairjr/RotaryDialUnified
