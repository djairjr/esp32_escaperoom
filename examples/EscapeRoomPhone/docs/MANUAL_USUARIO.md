# Escape Room Phone Manual

## What is this system?

This is an **interactive phone for Escape Room games**. Its main purpose is to **provide hints and information through audio messages** when players dial specific numbers.

There are **two models** available:
- **Keypad Model:** With a modern numeric keypad (calculator style)
- **Rotary Dial Model:** With a rotary dial (old-style phones)

> The system can be installed in an existing phone or mounted in a custom box.

---

## How does the game work?

### 1. Lift the handset off the hook

When they lift the phone, players hear a **dial tone** (the familiar "tu-tu-tu" of old phones). This indicates that the system is ready.

### 2. Dial a number

Players dial a number on the keypad (or turn the dial, on the rotary version):

- The system accepts numbers of up to **8 digits**
- If they stop dialing for **3 seconds**, the number is processed automatically
- Each dialed or keyed digit emits the corresponding sound (PULSE for the rotary dial or DTMF for the keypad)

### 3. Listen to the message

The phone plays an audio message corresponding to the dialed number:

| Result | What happens |
|-----------|----------------|
| **Valid number** | Plays an audio message (hint, instruction, story) |
| **Unregistered number** | Plays "this number does not exist" |

> **Example:** Dialing `1234` plays the message associated with that number.

### 4. Special actions (optional)

The system has a **relay-controlled output** that can be triggered to perform special actions, such as:
- Unlocking a lock
- Triggering a mechanism
- Revealing a compartment

**To activate the special action:**
Simply save a file with a `.txt` extension for a number that already has the `.mp3` file. For example, to open a door when dialing `123`, you need two files:
- `123.mp3` — the audio message
- `123.txt` — file that indicates the relay must be triggered (the content does not matter)

### 5. Hang up the phone

To make a new query, players must **put the handset back on the hook**. This resets the system automatically.

---

## For the Game Organizer

### Configuring the messages

The system works as an **interactive audio center**. You define which messages will be played for each number.

**Usage ideas:**

| Scenario | How to use |
|---------|-----------|
| **Hint center** | Numbers scattered around the room reveal hints when dialed |
| **Management phone** | A number "calls the boss" and gives the next instruction |
| **Hotline** | Only one specific number works (e.g. emergency) |
| **Narrative** | Each number represents a different character with useful information |

---

## Memory Card Configuration

The system uses an **SD card** to store the custom messages.

### Files on the SD card

The system sounds (dial tone, key beeps, "number does not exist" message) are already configured from the factory and cannot be changed.

On the SD card, you only need to put the **custom messages** you create:

| File | Function |
|---------|--------|
| `[number].mp3` | Message for each number (e.g. `1234.mp3`) |
| `[number].txt` | File that activates the relay output (optional) |

### Audio file format

- **Format:** MP3
- **Bit rate:** 128 kbps (recommended)
- **Frequency:** 44 kHz
- **Channels:** Stereo

> **Note:** Files with ID3 tags may not work correctly. It is recommended to remove the tags before use.

### SD card format

- **File system:** FAT32
- **Maximum capacity:** 32 GB
- **Recommended size:** 2-8 GB are enough

### Card compatibility

**Work well:**
- Class 4, 6 or 10 SD cards
- Known brands (SanDisk, Kingston, Samsung)

**May have problems:**
- SDXC cards (above 32 GB)
- High-speed cards (UHS-I, UHS-II)
- Very old cards (less than 1 GB)
- Generic microSD to SD adapters

> **Tip:** If the system does not recognize the card, try another model or format it again in FAT32.

---

## Auxiliary Relay Output

The system has a **relay output** to connect external devices:

- **Electric locks** — unlocks a door when the correct number is dialed
- **Lights** — triggers special lighting
- **Motors** — activates mechanisms or movements
- **Solenoids** — opens drawers or secret compartments

### Relay wiring

The output cable has three wires:
| Color | Function |
|-----|--------|
| **Blue** | Common (C) |
| **Black** | Normally Open (NO) |
| **Brown** | Normally Closed (NC) |

> Connect the **blue** wire to the common of the lock, and the **black** or **brown** according to the desired triggering type.

### How to activate the output

For a number to trigger the relay, create two files with the same name on the SD card:
1. `[number].mp3` — the audio message
2. `[number].txt` — file that indicates the relay must be triggered (the content does not matter)

### Lock precautions

**Power supply:** Use a quality power supply to feed the lock. Poor or low-capacity supplies can cause voltage drops that interfere with the phone's operation.

**Circuit protection:** Locks and solenoids must have a **flyback diode** (also called a freewheeling diode) installed in parallel with the coil. This protects the phone's circuit against voltage spikes generated when the relay turns off.

> **Important:** Without the proper flyback diode, the phone may malfunction or even suffer permanent damage.

## Troubleshooting

### The phone does not emit sound
1. Check that the phone is connected to power
2. Confirm that the handset is off the hook
3. Wait a few seconds after lifting the handset
4. Check that the SD card is inserted correctly

### The keypad/dial does not respond
1. Make sure the handset is off the hook
2. Check whether there is a dial tone (indicates it is ready)
3. Try dialing more slowly

### The lock does not trigger
1. Check that the files are correct — you need **both** files `[number].mp3` and `[number].txt` for the relay to trigger
2. Test the number and wait for the message to finish — the relay only triggers at the end of playback
3. Check the lock's power supply — weak supplies may not provide enough current
4. Listen to the relay — when triggered, it makes a **characteristic switching noise** (mechanical click)
5. Check the wire connections — blue on common, black or brown according to the triggering type

### Poor or distorted sound
1. Check for obstacles blocking the speaker
2. Check the quality of the MP3 files — convert them again if necessary
3. In case of constant hissing, check the power supply

### Messages do not appear
1. Confirm that the MP3 files are in the root of the SD card
2. Check that the file names exactly match the dialed numbers
3. Make sure the format is MP3 and not another audio format

---

## Tips for a good experience

### For the players:
- **Listen carefully** — the messages do not repeat automatically
- **Write down the information** — important hints may be in the messages
- **Explore numbers** — apparently random numbers in the scene may be useful
- **If unsure, try** — wrong numbers are not penalized, they only say "does not exist"

### For the organizer:
- **Test all the numbers** before the game
- **Have a backup** of the important messages
- **Prepare the hook** — make sure the physical sensor is working well
- **Use reliable SD cards** — low-quality cards may fail during the game

---

## Specifications

| Feature | Description |
|---------|-----------|
| **Input** | 4x3 numeric keypad or rotary dial |
| **Capacity** | Up to 8 digits per number |
| **Timeout** | 3 seconds without dialing |
| **Audio** | Custom messages in MP3 |
| **Optional output** | Can trigger locks or devices |
| **Storage** | SD card up to 32 GB (FAT32) |

---

## Support

For technical problems or configuration questions, consult the system's technical documentation or the manufacturer.

---

**Version:** 1.3  
**Date:** 2026-04-23
