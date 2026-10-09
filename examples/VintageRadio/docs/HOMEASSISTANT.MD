# VintageRadio — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, tuning, authenticated, solved |
| Frequency | `frequency` | 0-1023 (encoder) |
| Last Tag | `last_tag` | text (UID hex) |

## Binary Sensors
| Name | ID |
|---|---|
| Power | `power` | on, off |

## Buttons
| Name | ID |
|---|---|
| Power On/Off | `power_toggle` |
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Target Frequency | `target_freq` | 0-1023 |
| Volume | `volume` | 0-255 |

## AP Mode
WiFi network "VintageRadio-XXXX" + portal at 192.168.4.1.
