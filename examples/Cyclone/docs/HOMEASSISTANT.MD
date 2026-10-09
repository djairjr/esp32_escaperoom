# Cyclone — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved |
| Round Atual | `round` | 1-N |
| Velocidade | `speed` | 0.0-1.0 |

## Botões
| Nome | ID |
|---|---|
| Iniciar | `start` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Rounds para Vencer | `rounds_to_win` | 1-10 |
| Tamanho Alvo | `target_size` | 1-8 |
| Velocidade Inicial | `start_speed` | 0.01-0.50 |
| LEDs no Anel | `neo_count` | 8-48 |

## AP Mode
Rede WiFi "Cyclone-XXXX" + portal em 192.168.4.1.
