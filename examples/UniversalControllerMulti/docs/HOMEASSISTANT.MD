# UniversalControllerMulti — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved |
| Puzzles Ativos | `active_puzzles` | 0-N |
| Puzzles Resolvidos | `solved_puzzles` | 0-N |

## Binários (por puzzle)
| Nome | ID |
|---|---|
| Puzzle 1..N | `puzzle_1`..`puzzle_N` |

## Botões
| Nome | ID |
|---|---|
| Iniciar Todos | `start_all` |
| Resetar Todos | `reset_all` |

## AP Mode
Rede WiFi "MultiUniv-XXXX" + portal em 192.168.4.1.
