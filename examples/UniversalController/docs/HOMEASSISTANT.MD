# UniversalController — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved, error |
| Modo Atual | `mode` | texto (definido por JSON) |
| Inputs Ativos | `active_inputs` | 0-N |

## Binários (por input)
| Nome | ID |
|---|---|
| Input 1..N | `input_1`..`input_N` |

## Botões
| Nome | ID |
|---|---|
| Iniciar | `start` |
| Resetar | `reset` |

## Seletores
| Nome | ID | Opções |
|---|---|---|
| Modo | `mode` | SINGLE, MULTI, SEQUENCE, RFID, KEYPAD, TOGGLE, TIMER |

## Números (dinâmicos por JSON)
| Nome | ID | Range |
|---|---|---|
| Timeout | `timeout` | 1-300s |
| Máx Tentativas | `max_attempts` | 1-9 |

## AP Mode
Rede WiFi "Universal-XXXX" + portal em 192.168.4.1.
