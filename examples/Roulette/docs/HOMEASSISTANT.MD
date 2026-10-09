# Roulette — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, spinning, decelerating, solved, miss |
| Setor Atual | `current_sector` | 1-N |
| Velocidade | `speed` | 0-100% |

## Botões
| Nome | ID |
|---|---|
| Girar | `spin` |
| Parar | `stop` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Setor Vencedor | `winning_sector` | 1-N |
| Nº Setores | `num_sectors` | 4-24 |

## AP Mode
Rede WiFi "Roulette-XXXX" + portal em 192.168.4.1.
