# ReactorControl — Painel de Controle Nuclear

Puzzle analógico de ajuste para Escape Room usando a biblioteca **PropMaker EscapeRoom**.

## Narrativa

O reator nuclear entrou em estado crítico. Três técnicos devem cooperar para ajustar os parâmetros em um painel de controle com 3 faces. Cada face possui uma **válvula** (potenciômetro) e um **gauge** (voltímetro analógico), mas a válvula de uma face **nunca** controla o gauge da mesma face — o mapeamento é aleatório e pode mudar a qualquer momento.

Além disso, cada gauge pode responder no sentido **normal** ou **reverso**, aumentando a confusão. Quando todos os gauges atingem a zona verde, os jogadores devem pressionar **3 botões simultaneamente** (dentro de 2 segundos) para confirmar. Se falharem, o painel se reconfigura.

## Hardware

| Componente | Modelo | Barramento | Endereço/Canal | Função |
|------------|--------|------------|----------------|--------|
| Microcontrolador | Adafruit PropMaker Feather RP2040 | — | — | Core |
| Expansor I/O | PCF8575 | I2C | 0x20 | 3 botões |
| ADC | ADS1115 | I2C | 0x48 | 3 potenciômetros |
| Driver PWM | PCA9685 | I2C | 0x40 | 3 voltímetros |
| NeoPixel | Strip externo | GPIO | PIN_EXTERNAL_NEOPIXEL | 3 LEDs de feedback |
| Trava | Eletroímã/Relê | GPIO | PIN_LOCK | Abertura na vitória |

### Mapeamento de Canais

| Face | Parâmetro | ADS (Entrada) | PCA (Saída) | Botão PCF | NeoPixel |
|------|-----------|----------------|-------------|-----------|----------|
| 1 | Temperatura do Núcleo | AIN0 | PWM0 | P0 | LED 0 |
| 2 | Pressão do Refrigerante | AIN1 | PWM1 | P1 | LED 1 |
| 3 | Inserção das Barras | AIN2 | PWM2 | P2 | LED 2 |

### Valores-Alvo (Zona Verde)

| Face | Target | Tolerância |
|------|--------|------------|
| 1 | 7.0 | ±0.5 |
| 2 | 5.0 | ±0.5 |
| 3 | 3.0 | ±0.5 |

## Mecânica

1. **Início**: Pressione o botão 1 para iniciar.
2. **Ajuste**: Gire as válvulas para mover os gauges. Observe que a válvula N **não** afeta o gauge N — ela afeta um gauge de outra face.
3. **Zona Verde**: Cada gauge deve indicar o valor-alvo dentro da tolerância.
4. **Confirmação**: Quando todos os 3 gauges estiverem na zona verde, uma janela de 2 segundos se abre. Todos os 3 botões devem ser pressionados nesse intervalo.
5. **Reconfiguração**: Se a confirmação falhar, o mapeamento válvula→gauge e a orientação (normal/reverso) de cada gauge são re-embaralhados. Os jogadores devem redescobrir as relações.
6. **Vitória**: Confirmação bem-sucedida abre a trava e toca som de vitória.

## Sons Necessários

Coloque os seguintes arquivos MP3 (128Kbps, 44KHz, sem ID3 tags) em LittleFS ou SD:

| Arquivo | Evento |
|---------|--------|
| `ready.mp3` | Puzzle iniciado |
| `alarm.mp3` | Gauge fora da zona verde |
| `stabilizing.mp3` | Gauge entrou na zona verde |
| `warning.mp3` | Todos os gauges na zona (tensão) |
| `button_press.mp3` | Botão de confirmação pressionado |
| `fail.mp3` | Confirmação falhou |
| `reshuffle.mp3` | Reconfiguração após falha |
| `victory.mp3` | Puzzle resolvido |

**Nota:** Use `ffmpeg` para gerar MP3s compatíveis:
```bash
ffmpeg -y -i input.wav -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3
```

## Compilação

```bash
cd <Arduino>/libraries/propmaker_escaperoom
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker examples/ReactorControl/ReactorControl.ino
```

## Dependências

- `PropMakerEscapeRoom` (esta biblioteca)
- `IoAbstraction`
- `TaskManager`
- `SwitchInput`
- `Adafruit ADS1X15`
- `PCA9685`

## Configuração

Edite as constantes no topo do sketch para ajustar:
- `TARGETS[]` — valores-alvo de cada gauge
- `TOLERANCE` — margem de erro aceitável
- `CONFIRM_WINDOW_MS` — tempo para pressionar os 3 botões (ms)
- `RESHUFFLE_DURATION_MS` — duração da animação de reconfiguração
- `ADS_CHANNELS[]` — canais do ADS1115
- `PCA_CHANNELS[]` — canais do PCA9685
- `BUTTON_PINS[]` — pinos do PCF8575
- `BUTTON_PIXELS[]` — índices dos NeoPixel

## Autor

Djair Guilherme — Nicolau dos Brinquedos
Abril 2026
