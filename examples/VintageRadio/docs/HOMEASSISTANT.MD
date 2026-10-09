# VintageRadio — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, tuning, authenticated, solved |
| Frequência | `frequency` | 0-1023 (encoder) |
| Última Tag | `last_tag` | texto (UID hex) |

## Binários
| Nome | ID |
|---|---|
| Power | `power` | on, off |

## Botões
| Nome | ID |
|---|---|
| Ligar/Desligar | `power_toggle` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Frequência Alvo | `target_freq` | 0-1023 |
| Volume | `volume` | 0-255 |

## AP Mode
Rede WiFi "VintageRadio-XXXX" + portal em 192.168.4.1.
