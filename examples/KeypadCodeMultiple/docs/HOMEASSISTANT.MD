# KeypadCodeMultiple — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, entering, solved, error |
| Tentativas | `attempts` | 0-max |
| Códigos Ativos | `active_codes` | 1-8 |

## Botões
| Nome | ID |
|---|---|
| Resetar | `reset` |

## Seletores
| Nome | ID | Opções |
|---|---|---|
| Código Ativo | `active_code` | 1..8 |

## Números
| Nome | ID | Range |
|---|---|---|
| Código N | `code_1`..`code_8` | 0-9999 |

## AP Mode
Rede WiFi "KeypadMulti-XXXX" + portal em 192.168.4.1.
