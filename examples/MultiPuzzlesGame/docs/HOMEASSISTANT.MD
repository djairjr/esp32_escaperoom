# MultiPuzzlesGame — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | waiting, level_1..level_5, complete |
| Current Level | `current_level` | 1-5 |
| Remaining Time | `remaining_time` | 0-7200s |
| Solved Puzzles | `solved_count` | 0-9 |

## Binary Sensors (per puzzle)
| Name | ID |
|---|---|
| Puzzle 1..9 | `puzzle_1`..`puzzle_9` |

## Buttons
| Name | ID |
|---|---|
| Start Game | `start_game` |
| Skip Level | `skip_level` |
| Reset All | `reset_all` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Total Time | `total_time` | 60-7200s |

## AP Mode
WiFi network "MultiPuzzle-XXXX" + portal at 192.168.4.1 with an overview of all levels.
