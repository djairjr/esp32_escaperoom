# EscapeRoomPhone — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, off_hook, dialing, playing, busy |
| Dialed Number | `dialed_number` | text (up to 8 digits) |
| Last Digit | `last_digit` | 0-9 |

## Binary Sensors
| Name | ID |
|---|---|
| Hook | `hook` | on_hook, off_hook |
| Lock | `lock` | locked, unlocked |

## Buttons
| Name | ID |
|---|---|
| Unlock | `unlock` |
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Dial Timeout | `dial_timeout` | 1-10s |
| Max Digits | `max_digits` | 1-8 |

## AP Mode
WiFi network "EscapePhone-XXXX" + portal at 192.168.4.1.
