# KeypadRfidLock — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, authenticating, solved |
| Última Tag | `last_tag` | texto (UID hex) |

## Binários
| Nome | ID |
|---|---|
| Trava | `lock` | locked, unlocked |

## Botões
| Nome | ID |
|---|---|
| Destravar | `unlock` |
| Resetar | `reset` |

## Seletores
| Nome | ID | Opções |
|---|---|---|
| Modo | `mode` | KEYPAD, RFID, BOTH |

## Números
| Nome | ID | Range |
|---|---|---|
| Código | `code` | 0-9999 |
| Timeout Trava | `lock_timeout` | 1-30s |

## AP Mode
Rede WiFi "RfidLock-XXXX" + portal em 192.168.4.1.
