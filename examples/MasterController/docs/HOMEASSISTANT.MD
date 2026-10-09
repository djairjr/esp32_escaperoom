# MasterController — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| Game State | `game_state` | waiting, running, complete, expired |
| Remaining Time | `remaining_time` | 0-7200s |
| Solved Puzzles | `solved_puzzles` | 0-N |

## Binary Sensors
| Name | ID |
|---|---|
| Main Door | `main_door` | locked, unlocked |
| Lights | `lights` | on, off |
| Smoke Generator | `smoke_gen` | on, off |
| Auxiliary 1..4 | `aux_1`..`aux_4` | on, off |

## Buttons
| Name | ID |
|---|---|
| Start Game | `start_game` |
| Pause Game | `pause_game` |
| Reset Game | `reset_game` |
| Open All Doors | `emergency_open` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Total Time | `total_time` | 60-7200s |
| Required Puzzles | `required_puzzles` | 1-32 |

## AP Mode
WiFi network "MasterControl-XXXX" + portal at 192.168.4.1 with a full dashboard.
