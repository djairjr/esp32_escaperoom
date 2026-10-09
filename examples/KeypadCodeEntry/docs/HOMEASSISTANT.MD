# KeypadCodeEntry — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, entering, checking, solved, error |
| Tentativas | `attempts` | 0-max |

## Botões
| Nome | ID |
|---|---|
| Resetar | `reset` |

## Números
| Nome | ID | Range |
|---|---|---|
| Código | `code` | 0-9999 |
| Máx Tentativas | `max_attempts` | 1-9 |

## AP Mode
Rede WiFi "KeypadCode-XXXX" + portal em 192.168.4.1.
