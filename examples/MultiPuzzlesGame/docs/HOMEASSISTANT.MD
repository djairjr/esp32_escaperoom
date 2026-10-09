# MultiPuzzlesGame — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | waiting, level_1..level_5, complete |
| Nível Atual | `current_level` | 1-5 |
| Tempo Restante | `remaining_time` | 0-7200s |
| Puzzles Resolvidos | `solved_count` | 0-9 |

## Binários (por puzzle)
| Nome | ID |
|---|---|
| Puzzle 1..9 | `puzzle_1`..`puzzle_9` |

## Botões
| Nome | ID |
|---|---|
| Iniciar Jogo | `start_game` |
| Pular Nível | `skip_level` |
| Resetar Tudo | `reset_all` |

## Números
| Nome | ID | Range |
|---|---|---|
| Tempo Total | `total_time` | 60-7200s |

## AP Mode
Rede WiFi "MultiPuzzle-XXXX" + portal em 192.168.4.1 com visão geral de todos os níveis.
