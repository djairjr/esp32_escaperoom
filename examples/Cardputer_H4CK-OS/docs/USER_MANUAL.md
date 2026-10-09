# H4CK-OS — User Manual (Game Master)

## Terminal Commands

```
help                      — command list
help <command>            — detailed help
clear                     — clears the terminal
logout                    — returns to login
```

## Network Commands

| Command | Description |
|---------|-----------|
| `scan` | Scans the local network (192.168.1.0/24) — lists IPs + MACs |
| `nmap <ip>` | Port scan of a specific target |
| `hydra <ip>` | Password brute force on a service |
| `sniff <ip>` | Intercepts the target's network traffic |

## Scan — known devices

The scan reveals devices on the network by their MAC addresses.
Note down the MACs found — some devices may be
hidden in the room with matching labels.

| IP | MAC | Hostname |
|----|-----|----------|
| 192.168.1.1 | 7C:9E:BD:3F:2A:00 | gateway |
| 192.168.1.10 | D4:3C:2E:11:AB:01 | management |
| 192.168.1.11 | D4:3C:2E:11:AB:02 | development |
| 192.168.1.12 | D4:3C:2E:11:AB:03 | joe-laptop |
| 192.168.1.13 | D4:3C:2E:11:AB:04 | shredder |
| 192.168.1.14 | D4:3C:2E:11:AB:05 | security |
| **192.168.1.100** | **F4:5C:89:AB:CD:EF** | **unknown** |

> The `unknown` device (MAC F4:5C:89:AB:CD:EF, IP 192.168.1.100)
> is the only unidentified host on the network. Custom ports
> (8080, 9090, 7070) suggest a non-standard device.

## RF433 Commands

| Command | Description |
|---------|-----------|
| `exploit -k <key>` | Generates an RF433 payload from a key |
| `hackcomm send <hex>` | Transmits an RF433 code (24 bits) |

### RF433 Codes (clues for the player)

| Function | Hex Code |
|--------|-----------|
| ARM | `0x456789` |
| DESTROY | `0x56789A` |
| DISARM | `0x6789AB` |
| BOOST | `0x789ABC` |
| FREEZE | `0x89ABCD` |

> Only `hackcomm send <hex>` is available in the terminal.
> The codes above are a reference for the GM to place as clues in the room.

## Phone Commands

| Command | Description |
|---------|-----------|
| `tapscan` | Lists the available phone channels |
| `tapscan listen <n>` | Plays the recording of channel n |

## System Commands

| Command | Description |
|---------|-----------|
| `invade list` | Lists the available payloads |
| `invade send <payload>` | Sends a payload (publishes to Home Assistant) |
| `status` | Checks the status of the target device |

## Home Assistant (Game Master)

The Cardputer exposes the following entities in Home Assistant:

| Type | ID | Description |
|------|----|-----------|
| Sensor | `h4ckos_status` | Terminal status |
| Sensor | `h4ckos_login_password` | Current login password |
| Sensor | `h4ckos_sniff_ip` | Target IP for sniff |
| Number | `h4ckos_defuse_code` | Access token (0-9999) |
| Number | `h4ckos_sniff_channel` | Audio channel (1-4) |
| Select | `h4ckos_stage` | Stage (SCAN/HYDRA/EXPLOIT/RF433) |
| Switch | `h4ckos_rf433_enable` | Enables RF433 transmission |
| Sensor | `h4ckos_last_payload` | Last PAYLOAD sent by the player |

## AP Mode (Configuration)

If WiFi is not configured, the Cardputer brings up a network:
- **SSID:** `H4CK-OS-Config`
- **Access:** http://192.168.4.1

Configure WiFi, MQTT, and reboot.
