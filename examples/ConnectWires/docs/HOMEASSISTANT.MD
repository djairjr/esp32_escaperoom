# ConnectWires — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved |
| Conexões Corretas | `correct_connections` | 0-N |
| Tensão Canal 0 | `voltage_0` | 0-5000mV |

## Binários (por soquete)
| Nome | ID |
|---|---|
| Soquete 1..N | `socket_1`..`socket_N` |

## Binários (por par de conexão)
| Nome | ID |
|---|---|
| Par A..D | `pair_a`..`pair_d` |

## Botões
| Nome | ID |
|---|---|
| Resetar | `reset` |
| Destravar | `unlock` |

## Números
| Nome | ID | Range |
|---|---|---|
| Nº Conexões | `num_connections` | 2-6 |

## AP Mode
Rede WiFi "ConnectWires-XXXX" + portal em 192.168.4.1.
