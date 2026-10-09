# EscapeRoomPhone — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, off_hook, dialing, playing, busy |
| Número Discado | `dialed_number` | texto (até 8 dígitos) |
| Último Dígito | `last_digit` | 0-9 |

## Binários
| Nome | ID |
|---|---|
| Gancho | `hook` | on_hook, off_hook |
| Trava | `lock` | locked, unlocked |

## Botões
| Nome | ID |
|---|---|
| Destravar | `unlock` |
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Timeout Discagem | `dial_timeout` | 1-10s |
| Máx Dígitos | `max_digits` | 1-8 |

## AP Mode
Rede WiFi "EscapePhone-XXXX" + portal em 192.168.4.1.
