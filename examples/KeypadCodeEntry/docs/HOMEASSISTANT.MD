# KeypadCodeEntry — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, entering, checking, solved, error |
| Attempts | `attempts` | 0-max |

## Buttons
| Name | ID |
|---|---|
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Code | `code` | 0-9999 |
| Max Attempts | `max_attempts` | 1-9 |

## AP Mode
WiFi network "KeypadCode-XXXX" + portal at 192.168.4.1.
