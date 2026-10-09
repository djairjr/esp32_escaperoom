# RadioClock — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, solved |
| Current Frequency | `frequency` | 0-frequencyCount |
| Current Round | `round` | 1-roundsToWin |
| Target Frequency | `target_freq` | 0-frequencyCount |

## Buttons
| Name | ID |
|---|---|
| Start | `start` |
| Reset | `reset` |
| Next Frequency | `next_freq` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Frequencies on Dial | `freq_count` | 4-24 |
| Rounds to Win | `rounds_to_win` | 1-10 |

## AP Mode
WiFi network "RadioClock-XXXX" + portal at 192.168.4.1.
