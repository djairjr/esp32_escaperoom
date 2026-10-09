# ConnectWires — Home Assistant Entities

## Sensors
| Name | ID | Values |
|---|---|---|
| State | `state` | idle, running, solved |
| Correct Connections | `correct_connections` | 0-N |
| Channel 0 Voltage | `voltage_0` | 0-5000mV |

## Binary sensors (per socket)
| Name | ID |
|---|---|
| Socket 1..N | `socket_1`..`socket_N` |

## Binary sensors (per connection pair)
| Name | ID |
|---|---|
| Pair A..D | `pair_a`..`pair_d` |

## Buttons
| Name | ID |
|---|---|
| Reset | `reset` |
| Unlock | `unlock` |

## Numbers
| Name | ID | Range |
|---|---|---|
| No. Connections | `num_connections` | 2-6 |

## AP Mode
WiFi network "ConnectWires-XXXX" + portal at 192.168.4.1.
