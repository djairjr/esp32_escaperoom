# UniversalControllerMulti — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, solved |
| Active Puzzles | `active_puzzles` | 0-N |
| Solved Puzzles | `solved_puzzles` | 0-N |

## Binary Sensors (per puzzle)
| Name | ID |
|---|---|
| Puzzle 1..N | `puzzle_1`..`puzzle_N` |

## Buttons
| Name | ID |
|---|---|
| Start All | `start_all` |
| Reset All | `reset_all` |

## AP Mode
WiFi network "MultiUniv-XXXX" + portal at 192.168.4.1.
