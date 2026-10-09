# SimonSays — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, showing, waiting, correct, wrong, solved |
| Current Round | `round` | 1-maxRounds |
| Correct Sequence | `sequence` | text (e.g. "R,B,G,Y") |
| Player Input | `player_input` | text |

## Binary sensors (per button)
| Name | ID |
|---|---|
| Red Button | `btn_red` |
| Blue Button | `btn_blue` |
| Green Button | `btn_green` |
| Yellow Button | `btn_yellow` |

## Buttons
| Name | ID |
|---|---|
| Start | `start` |
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Max Rounds | `max_rounds` | 3-50 |
| Time Between Notes | `note_interval` | 200-2000ms |

## AP Mode
WiFi network "SimonSays-XXXX" + portal at 192.168.4.1.
