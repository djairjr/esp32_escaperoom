# RadioClock — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved |
| Frequência Atual | `frequency` | 0-frequencyCount |
| Round Atual | `round` | 1-roundsToWin |
| Frequência Alvo | `target_freq` | 0-frequencyCount |

## Botões
| Nome | ID |
|---|---|
| Iniciar | `start` |
| Resetar | `reset` |
| Próxima Frequência | `next_freq` |

## Números
| Nome | ID | Range |
|---|---|---|
| Frequências no Dial | `freq_count` | 4-24 |
| Rounds para Vencer | `rounds_to_win` | 1-10 |

## AP Mode
Rede WiFi "RadioClock-XXXX" + portal em 192.168.4.1.
