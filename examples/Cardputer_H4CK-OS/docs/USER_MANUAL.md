# H4CK-OS — Manual do Usuário (Game Master)

## Comandos do Terminal

```
help                      — lista de comandos
help <comando>            — ajuda detalhada
clear                     — limpa terminal
logout                    — volta ao login
```

## Comandos de Rede

| Comando | Descrição |
|---------|-----------|
| `scan` | Escaneia rede local (192.168.1.0/24) — lista IPs + MACs |
| `nmap <ip>` | Port scan de um alvo específico |
| `hydra <ip>` | Força bruta de senha em serviço |
| `sniff <ip>` | Intercepta tráfego de rede do alvo |

## Scan — dispositivos conhecidos

O scan revela dispositivos na rede pelos seus endereços MAC.
Anote os MACs encontrados — alguns dispositivos podem estar
escondidos na sala com etiquetas correspondentes.

| IP | MAC | Hostname |
|----|-----|----------|
| 192.168.1.1 | 7C:9E:BD:3F:2A:00 | gateway |
| 192.168.1.10 | D4:3C:2E:11:AB:01 | gerencia |
| 192.168.1.11 | D4:3C:2E:11:AB:02 | desenvolvimento |
| 192.168.1.12 | D4:3C:2E:11:AB:03 | joe-laptop |
| 192.168.1.13 | D4:3C:2E:11:AB:04 | ralador |
| 192.168.1.14 | D4:3C:2E:11:AB:05 | seguranca |
| **192.168.1.100** | **F4:5C:89:AB:CD:EF** | **unknown** |

> O dispositivo `unknown` (MAC F4:5C:89:AB:CD:EF, IP 192.168.1.100)
> é o único host não identificado na rede. Portas customizadas
> (8080, 9090, 7070) sugerem um dispositivo não-padrão.

## Comandos RF433

| Comando | Descrição |
|---------|-----------|
| `exploit -k <key>` | Gera payload RF433 a partir de uma chave |
| `hackcomm send <hex>` | Transmite código RF433 (24 bits) |

### Códigos RF433 (pistas para o jogador)

| Função | Código Hex |
|--------|-----------|
| ARM | `0x456789` |
| DESTROY | `0x56789A` |
| DISARM | `0x6789AB` |
| BOOST | `0x789ABC` |
| FREEZE | `0x89ABCD` |

> Apenas `hackcomm send <hex>` está disponível no terminal.
> Os códigos acima são referência para o GM colocar como pistas na sala.

## Comandos de Telefone

| Comando | Descrição |
|---------|-----------|
| `tapscan` | Lista canais de telefone disponíveis |
| `tapscan listen <n>` | Toca a gravação do canal n |

## Comandos de Sistema

| Comando | Descrição |
|---------|-----------|
| `invade list` | Lista payloads disponíveis |
| `invade send <payload>` | Envia payload (publica no Home Assistant) |
| `status` | Verifica status do dispositivo alvo |

## Home Assistant (Game Master)

O Cardputer expõe as seguintes entidades no Home Assistant:

| Tipo | ID | Descrição |
|------|----|-----------|
| Sensor | `h4ckos_status` | Status do terminal |
| Sensor | `h4ckos_login_password` | Senha atual do login |
| Sensor | `h4ckos_sniff_ip` | IP alvo do sniff |
| Number | `h4ckos_defuse_code` | Token de acesso (0-9999) |
| Number | `h4ckos_sniff_channel` | Canal de áudio (1-4) |
| Select | `h4ckos_stage` | Estágio (SCAN/HYDRA/EXPLOIT/RF433) |
| Switch | `h4ckos_rf433_enable` | Habilita transmissão RF433 |
| Sensor | `h4ckos_last_payload` | Último PAYLOAD enviado pelo jogador |

## AP Mode (Configuração)

Se o WiFi não estiver configurado, o Cardputer sobe uma rede:
- **SSID:** `H4CK-OS-Config`
- **Acesso:** http://192.168.4.1

Configure WiFi, MQTT, e reinicie.
