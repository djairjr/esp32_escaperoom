# SimonSays — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, showing, waiting, correct, wrong, solved |
| Round Atual | `round` | 1-maxRounds |
| Sequência Correta | `sequence` | texto (ex: "R,B,G,Y") |
| Entrada Jogador | `player_input` | texto |

## Binários (por botão)
| Nome | ID |
|---|---|
| Botão Vermelho | `btn_red` |
| Botão Azul | `btn_blue` |
| Botão Verde | `btn_green` |
| Botão Amarelo | `btn_yellow` |

## Botões
| Nome | ID |
|---|---|
| Iniciar | `start` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Max Rounds | `max_rounds` | 3-50 |
| Tempo Entre Notas | `note_interval` | 200-2000ms |

## AP Mode
Rede WiFi "SimonSays-XXXX" + portal em 192.168.4.1.
