# KeypadRfidLock — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, authenticating, solved |
| Last Tag | `last_tag` | text (hex UID) |

## Binaries
| Name | ID |
|---|---|
| Lock | `lock` | locked, unlocked |

## Buttons
| Name | ID |
|---|---|
| Unlock | `unlock` |
| Reset | `reset` |

## Selectors
| Name | ID | Options |
|---|---|---|
| Mode | `mode` | KEYPAD, RFID, BOTH |

## Numbers
| Name | ID | Range |
|---|---|---|
| Code | `code` | 0-9999 |
| Lock Timeout | `lock_timeout` | 1-30s |

## AP Mode
WiFi network "RfidLock-XXXX" + portal at 192.168.4.1.
