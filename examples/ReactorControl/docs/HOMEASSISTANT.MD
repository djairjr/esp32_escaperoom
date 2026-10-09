# ReactorControl — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, stable, warning, critical, solved, meltdown |
| Temperature | `temperature` | 0-100% |
| Pressure | `pressure` | 0-100% |
| Remaining Time | `remaining_time` | 0-totalTime |

## Binary Sensors
| Name | ID |
|---|---|
| Venting | `vent` | on, off |
| Valve | `valve` | on, off |
| SCRAM | `scram` | active, inactive |

## Buttons
| Name | ID |
|---|---|
| Start | `start` |
| SCRAM | `emergency_scram` |
| Reset | `reset` |

## Numbers
| Name | ID | Range |
|---|---|---|
| Temperature Limit | `max_temp` | 50-100 |
| Pressure Limit | `max_pressure` | 50-100 |
| Total Time | `total_time` | 30-300s |

## AP Mode
WiFi network "Reactor-XXXX" + portal at 192.168.4.1.
