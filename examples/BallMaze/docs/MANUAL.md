# Manual do BallMaze - Labirinto com Acelerometro

Guia para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

O **BallMaze** e um puzzle de labirinto fisico virtual. A caixa contem uma placa PropMaker com acelerometro LIS3DH que detecta a inclinacao da caixa. Uma "bola" virtual se move na tela de LEDs na direcao da inclinacao. O jogador deve guiar a bola ate todas as zonas-alvo para resolver o puzzle.

### Como funciona no jogo?

- Os jogadores encontram uma caixa com luzes LED
- Ao inclinar a caixa, uma bola virtual se move na direcao da inclinacao
- O objetivo e visitar todas as zonas-alvo (marcadas em amarelo nos LEDs)
- Quando a bola passa por uma zona-alvo, ela fica verde e um som e tocado
- Ao visitar todas as zonas, o puzzle e resolvido

---

## 2. Componentes do Jogo

| Componente | Descricao |
|------------|-----------|
| **LEDs NeoPixel** | Fileira de luzes que mostra a posicao da bola e das zonas-alvo |
| **Caixa** | O proprio corpo do dispositivo - deve ser inclinada |
| **Alto-falante** | Emite sons de movimento, acerto e vitoria |

---

## 3. Configuracao Inicial

### Antes de usar pela primeira vez

1. Carregue os arquivos de audio no LittleFS ou SD Card
2. Ajuste o arquivo `/ballmaze.json` se necessario (veja secao 8)
3. Ligue o dispositivo

### Verificacao de funcionamento

1. Ligue o dispositivo
2. Os LEDs devem acender em azul suave e pulsante
3. Incline a caixa - a bola (LED branco) deve se mover na direcao da inclinacao
4. Se os LEDs piscarem vermelho, o acelerometro nao foi detectado

---

## 4. Como Jogar

### Para o Game Master - Iniciando o jogo

1. Coloque a caixa em uma superficie plana
2. Explique aos jogadores que devem inclinar a caixa para mover a bola
3. Quando estiverem prontos, peca para sacudirem levemente a caixa para comecar
4. O puzzle esta em andamento quando a bola comeca a se mover

### Para os jogadores - Durante o jogo

1. **Inicie**: Sacuda a caixa levemente para comecar
2. **Mova a bola**: Incline a caixa na direcao que quer que a bola va
3. **Encontre os alvos**: Guie a bola ate as zonas amarelas
4. **Veja o progresso**: Cada zona visitada fica verde
5. **Venca**: Quando todas as zonas estiverem verdes, o puzzle e resolvido!

### Dicas de jogabilidade

- Inclinacoes suaves funcionam melhor que movimentos bruscos
- A bola tem inercia - ela continua se movendo um pouco apos parar de inclinar
- Se a bola bater na borda, ela quica de volta
- Agite a caixa para comecar um novo jogo apos vencer

---

## 5. O que acontece ao resolver

Quando todas as zonas-alvo sao visitadas:

1. Os LEDs fazem um efeito rainbow (todas as cores)
2. O som de vitoria toca
3. A trava (se configurada) e liberada
4. Apos 10 segundos, o puzzle volta ao estado inicial

---

## 6. Configuracao Avancada (ballmaze.json)

### Parametros de Fisica

| Parametro | O que faz | Valor Recomendado |
|-----------|-----------|-------------------|
| `phys.accel` | Sensibilidade a inclinacao | 600 (mais = mais rapido) |
| `phys.damp` | Atrito da bola | 3.5 (mais = para mais rapido) |
| `phys.bounce` | Quique nas paredes | 0.3 (0 = nao quica, 1 = quica total) |
| `phys.maxSpd` | Velocidade maxima | 250 |
| `phys.deadZ` | Zona morta (ignora ruido) | 0.25 (menos = mais sensivel) |

### Dicas de ajuste

- **Bola muito lenta**: Aumente `phys.accel` ou diminua `phys.damp`
- **Bola muito rapida**: Diminua `phys.accel` ou `phys.maxSpd`
- **Bola treme parada**: Aumente `phys.deadZ`
- **Alvos muito dificeis**: Aumente `targetRadius`
- **Alvos muito faceis**: Diminua `targetRadius`

### Posicao das zonas-alvo

O arquivo JSON contem um array `targets` com posicoes X, Y e raio:

```json
"targets": [
    {"x": 30, "y": 30, "r": 12},
    {"x": 170, "y": 30, "r": 12},
    {"x": 100, "y": 170, "r": 12}
]
```

O labirinto virtual tem 200x200 unidades. X=0 e Y=0 e o canto superior esquerdo.

---

## 7. Checklist de Audio

| Arquivo | Quando Toca | Obrigatorio? |
|---------|-------------|--------------|
| `ball_roll.mp3` | Enquanto a bola se move | Sim |
| `target_hit.mp3` | Ao visitar uma zona-alvo | Sim |
| `solved.mp3` | Puzzle resolvido | Sim |
| `idle_chime.mp3` | A cada 15s no idle (opcional) | Nao |

### Formato dos arquivos

- **Formato**: MP3
- **Taxa de bits**: 128 kbps
- **Frequencia**: 44.1 kHz
- **Canais**: Stereo
- **Sem tags ID3**

---

## 8. Solucao de Problemas

### A bola nao se move

1. Verifique se o acelerometro esta funcionando (hw.test() no Serial)
2. Aumente `phys.accel` no JSON
3. Diminua `phys.deadZ` para maior sensibilidade

### A bola treme sem mexer

1. Aumente `phys.deadZ` no JSON
2. Coloque a caixa em uma superficie mais estavel
3. Verifique se nao ha vibracao externa

### LEDs vermelhos piscando

- O acelerometro LIS3DH nao foi detectado
- Verifique conexao I2C (endereco 0x18)
- Pode ser necessario soldar ou substituir a placa

### O puzzle nao inicia

1. Agite a caixa com mais forca (threshold minimo de 15 m/s^2)
2. Verifique no Serial se o shake esta sendo detectado
3. Diminua o threshold no codigo (se necessario)

---

## Resumo Rapido

| Acao | Como Fazer |
|------|------------|
| Iniciar jogo | Sacuda a caixa |
| Mover a bola | Incline a caixa |
| Visitar alvo | Passe a bola sobre a zona amarela |
| Resolver puzzle | Visite todas as zonas-alvo |
| Reiniciar | Apos solved, aguarde 10s ou sacuda |

### Estados dos LEDs

| Cor/Padrao | Significado |
|------------|-------------|
| Azul suave pulsante | Aguardando inicio (idle) |
| Branco + amarelos | Jogo em andamento |
| Branco + verdes | Alvos ja visitados |
| Rainbow | Puzzle resolvido! |
| Vermelho piscando | Erro: acelerometro ausente |

---

*Bom jogo! Nao deixe a bola cair...*

*Manual BallMaze - Versao 1.0 para biblioteca PropMaker EscapeRoom*
