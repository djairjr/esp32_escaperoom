# KeypadCodeMultiple — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, entering, solved, error |
| Attempts | `attempts` | 0-max |
| Active Codes | `active_codes` | 1-8 |

## Buttons
| Name | ID |
|---|---|
| Reset | `reset` |

## Selectors
| Name | ID | Options |
|---|---|---|
| Active Code | `active_code` | 1..8 |

## Numbers
| Name | ID | Range |
|---|---|---|
| Code N | `code_1`..`code_8` | 0-9999 |

## AP Mode
WiFi network "KeypadMulti-XXXX" + portal at 192.168.4.1.
