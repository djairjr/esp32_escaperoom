# Mastermind — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, solved, gameover |
| Current Attempt | `current_attempt` | 1-maxAttempts |
| Correct Position Colors | `exact_matches` | 0-codeLength |
| Wrong Position Colors | `color_matches` | 0-codeLength |

## Buttons
| Name | ID |
|---|---|
| Reset | `reset` |
| New Game | `new_game` |

## Selectors (per position)
| Name | ID | Options |
|---|---|---|
| Position 1..8 | `pos_1`..`pos_8` | Red, Blue, Green, Yellow, Magenta, Cyan |

## Numbers
| Name | ID | Range |
|---|---|---|
| Code Length | `code_length` | 2-8 |
| Max Attempts | `max_attempts` | 3-20 |
| No. Colors | `num_colors` | 2-6 |

## AP Mode
WiFi network "Mastermind-XXXX" + portal at 192.168.4.1.
