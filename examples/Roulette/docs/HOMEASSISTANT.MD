# Roulette — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, spinning, decelerating, solved, miss |
| Current Sector | `current_sector` | 1-N |
| Speed | `speed` | 0-100% |

## Buttons
| Name | ID |
|---|---|
| Spin | `spin` |
| Stop | `stop` |
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Winning Sector | `winning_sector` | 1-N |
| Num Sectors | `num_sectors` | 4-24 |

## AP Mode
WiFi network "Roulette-XXXX" + portal at 192.168.4.1.
