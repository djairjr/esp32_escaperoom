# BallMaze — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved |
| Alvos Atingidos | `targets_hit` | 0-N |
| Posição Bola X | `ball_x` | 0-mazeW |
| Posição Bola Y | `ball_y` | 0-mazeH |

## Binários (por alvo)
| Nome | ID |
|---|---|
| Alvo 1..8 | `target_1`..`target_8` |

## Botões
| Nome | ID |
|---|---|
| Iniciar | `start` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Sensibilidade | `accel_scale` | 100-1000 |
| Amortecimento | `damping` | 1-10 |
| Nº Alvos | `target_count` | 2-8 |

## AP Mode
Rede WiFi "BallMaze-XXXX" + portal em 192.168.4.1.
