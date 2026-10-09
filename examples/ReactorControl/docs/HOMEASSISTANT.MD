# ReactorControl — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, stable, warning, critical, solved, meltdown |
| Temperatura | `temperature` | 0-100% |
| Pressão | `pressure` | 0-100% |
| Tempo Restante | `remaining_time` | 0-totalTime |

## Binários
| Nome | ID |
|---|---|
| Ventilação | `vent` | on, off |
| Válvula | `valve` | on, off |
| SCRAM | `scram` | active, inactive |

## Botões
| Nome | ID |
|---|---|
| Iniciar | `start` |
| SCRAM | `emergency_scram` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Limite Temperatura | `max_temp` | 50-100 |
| Limite Pressão | `max_pressure` | 50-100 |
| Tempo Total | `total_time` | 30-300s |

## AP Mode
Rede WiFi "Reactor-XXXX" + portal em 192.168.4.1.
