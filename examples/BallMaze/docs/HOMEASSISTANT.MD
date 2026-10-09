# BallMaze — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, solved |
| Targets Hit | `targets_hit` | 0-N |
| Ball Position X | `ball_x` | 0-mazeW |
| Ball Position Y | `ball_y` | 0-mazeH |

## Binary sensors (per target)
| Name | ID |
|---|---|
| Target 1..8 | `target_1`..`target_8` |

## Buttons
| Name | ID |
|---|---|
| Start | `start` |
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Sensitivity | `accel_scale` | 100-1000 |
| Damping | `damping` | 1-10 |
| No. Targets | `target_count` | 2-8 |

## AP Mode
WiFi network "BallMaze-XXXX" + portal at 192.168.4.1.
