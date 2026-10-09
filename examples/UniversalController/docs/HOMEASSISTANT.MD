# UniversalController — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, solved, error |
| Current Mode | `mode` | text (defined by JSON) |
| Active Inputs | `active_inputs` | 0-N |

## Binary (per input)
| Name | ID |
|---|---|
| Input 1..N | `input_1`..`input_N` |

## Buttons
| Name | ID |
|---|---|
| Start | `start` |
| Reset | `reset` |

## Selectors
| Name | ID | Options |
|---|---|---|
| Mode | `mode` | SINGLE, MULTI, SEQUENCE, RFID, KEYPAD, TOGGLE, TIMER |

## Numbers (dynamic per JSON)
| Name | ID | Range |
|---|---|---|
| Timeout | `timeout` | 1-300s |
| Max Attempts | `max_attempts` | 1-9 |

## AP Mode
WiFi network "Universal-XXXX" + portal at 192.168.4.1.
