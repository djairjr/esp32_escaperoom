# BombTimer — Entidades Home Assistant

## Dispositivo
- **Nome**: BombTimer STAMPLC (configurável)
- **Conexão**: MQTT via ArduinoHA
- **Configuração inicial**: AP Mode (WiFi "BombTimer-STAMPLC" + portal web em 192.168.4.1)

## Sensores (HASensor)

| Nome | ID | Tipo | Valores | Ícone |
|---|---|---|---|---|
| Estado do Jogo | `state` | texto | idle, running, defused, detonated, menu, alarm | `mdi:bomb` |
| Tempo Restante | `time` | texto (s) | 0-5999 | `mdi:timer` |
| Fios Cortados | `wires_cut` | texto | 0-4 | `mdi:cable-data` |
| Tentativas | `code_attempts` | texto | 0-maxCodeAttempts | `mdi:counter` |
| I2C Devices | `i2c_status` | texto | lista de dispositivos detectados | `mdi:chip` |

## Sensores Binários (HASensor por fio)

| Nome | ID | Valores | Ícone |
|---|---|---|---|
| Fio Amarelo | `wire_yellow` | 0=connected, 1=cut | `mdi:cable-data` |
| Fio Vermelho | `wire_red` | 0=connected, 1=cut | `mdi:cable-data` |
| Fio Azul | `wire_blue` | 0=connected, 1=cut | `mdi:cable-data` |
| Fio Verde | `wire_green` | 0=connected, 1=cut | `mdi:cable-data` |

## Botões (HAButton)

| Nome | ID | Ação | Ícone |
|---|---|---|---|
| Iniciar | `start` | Inicia contagem regressiva (IDLE→RUNNING) | `mdi:play` |
| Resetar | `reset` | Reseta para IDLE (qualquer estado) | `mdi:restart` |

## Números (HANumber)

| Nome | ID | Range | Step | Unidade | Ícone |
|---|---|---|---|---|---|
| Ajuste de Tempo | `countdown` | 5-5999 | 1 | s | `mdi:timer-outline` |
| Código Desarme | `defuse_code` | 0-9999 | 1 | — | `mdi:lock` |
| Brilho | `brightness` | 0-15 | 1 | — | `mdi:brightness-6` |

## Seletores (HASelect)

| Nome | ID | Opções | Ícone |
|---|---|---|---|
| Modo Desarme | `mode` | CÓDIGO, FIO, ORDEM | `mdi:tune` |
| Fio Detonar | `det_wire` | ALEATÓRIO, Amarelo, Vermelho, Azul, Verde, NENHUM | `mdi:alert` |
| Fio Desarme | `defuse_wire` | ALEATÓRIO, Amarelo, Vermelho, Azul, Verde, NENHUM | `mdi:shield-check` |
| Fio Velocidade 2x | `speed_wire` | ALEATÓRIO, Amarelo, Vermelho, Azul, Verde, NENHUM | `mdi:speedometer` |
| Fio Pausar | `pause_wire` | ALEATÓRIO, Amarelo, Vermelho, Azul, Verde, NENHUM | `mdi:pause` |
| Ordem Amarelo | `order_yellow` | 1, 2, 3, 4 | `mdi:format-list-numbered` |
| Ordem Vermelho | `order_red` | 1, 2, 3, 4 | `mdi:format-list-numbered` |
| Ordem Azul | `order_blue` | 1, 2, 3, 4 | `mdi:format-list-numbered` |
| Ordem Verde | `order_green` | 1, 2, 3, 4 | `mdi:format-list-numbered` |

## Switch (HASwitch)

| Nome | ID | Ícone |
|---|---|---|
| Som Tick | `tick_sound` | `mdi:volume-high` |

## Comportamento MQTT

- **Publicação inicial**: ao conectar, publica todos os estados
- **Publicação periódica**: a cada 2 segundos durante RUNNING
- **Publicação sob demanda**: ao mudar estado, cortar fio, inserir código
- **Comandos recebidos**: iniciar, resetar, alterar configuração

## Automações Sugeridas

1. **Notificação de detonação**: quando `state` muda para `detonated` → notificar GameMaster
2. **Notificação de desarme**: quando `state` muda para `defused` → notificar GameMaster
3. **Sincronização de timer**: quando `start` é pressionado → iniciar timer global na sala
4. **Alerta de fio cortado**: quando qualquer `wire_*` muda para 1 → log no HA
5. **Ajuste remoto**: GameMaster altera `countdown`, `defuse_code` ou `mode` via HA

## Configuração de Rede (AP Mode)

1. Na primeira inicialização, cria rede WiFi "BombTimer-STAMPLC" (aberta)
2. Portal web em `http://192.168.4.1` permite configurar:
   - WiFi (SSID + senha)
   - MQTT (broker IP, porta, usuário, senha)
   - Nome do dispositivo no HA
3. Salva em `/wifi.json` no LittleFS e reinicia
4. Se WiFi falhar após 20 tentativas, volta ao modo AP
