# Mastermind — Entidades Home Assistant

## Sensores
| Nome | ID | Valores |
|---|---|---|
| Estado | `state` | idle, running, solved, gameover |
| Tentativa Atual | `current_attempt` | 1-maxAttempts |
| Cores Posição Certa | `exact_matches` | 0-codeLength |
| Cores Posição Errada | `color_matches` | 0-codeLength |

## Botões
| Nome | ID |
|---|---|
| Resetar | `reset` |
| Novo Jogo | `new_game` |

## Seletores (por posição)
| Nome | ID | Opções |
|---|---|---|
| Posição 1..8 | `pos_1`..`pos_8` | Vermelho, Azul, Verde, Amarelo, Magenta, Ciano |

## Números
| Nome | ID | Range |
|---|---|---|
| Tamanho Código | `code_length` | 2-8 |
| Máx Tentativas | `max_attempts` | 3-20 |
| Nº Cores | `num_colors` | 2-6 |

## AP Mode
Rede WiFi "Mastermind-XXXX" + portal em 192.168.4.1.
