# UniversalUniqueRFID — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, learning, ready, authenticating, solved |
| Programmed Tag | `programmed_tag` | text (hex UID) |
| Last Tag Read | `last_tag` | text (hex UID) |

## Buttons
| Name | ID |
|---|---|
| Learn Mode | `learn_mode` |
| Reset | `reset` |
| Clear Tag | `clear_tag` |

## AP Mode
WiFi network "UniqueRFID-XXXX" + portal at 192.168.4.1.
