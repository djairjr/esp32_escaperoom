# MasterController — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado do Jogo | `game_state` | waiting, running, complete, expired |
| Tempo Restante | `remaining_time` | 0-7200s |
| Puzzles Resolvidos | `solved_puzzles` | 0-N |

## Binários
| Nome | ID |
|---|---|
| Porta Principal | `main_door` | locked, unlocked |
| Luzes | `lights` | on, off |
| Gerador Fumaça | `smoke_gen` | on, off |
| Auxiliar 1..4 | `aux_1`..`aux_4` | on, off |

## Botões
| Nome | ID |
|---|---|
| Iniciar Jogo | `start_game` |
| Pausar Jogo | `pause_game` |
| Resetar Jogo | `reset_game` |
| Abrir Todas Portas | `emergency_open` |

## Números
| Nome | ID | Range |
|---|---|---|
| Tempo Total | `total_time` | 60-7200s |
| Puzzles Necessários | `required_puzzles` | 1-32 |

## AP Mode
Rede WiFi "MasterControl-XXXX" + portal em 192.168.4.1 com dashboard completo.
