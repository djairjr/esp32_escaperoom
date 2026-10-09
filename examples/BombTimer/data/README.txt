Inclua os seguintes arquivos MP3 na memória LittleFS (via Arduino Pico LittleFS Uploader):

SONS OBRIGATÓRIOS:
- tick.mp3           Tique-taque a cada segundo (pode ser desativado no menu "tic")
- armed.mp3          Toca ao armar a bomba (botão DETONATE no Idle)
- boom.mp3           Toca ao detonar
- success.mp3        Toca ao desarmar com sucesso
- keypress.mp3       Bip curto a cada tecla pressionada e botão de menu
- error.mp3          Toca quando código errado, shake-pause ou erro crítico
- menu_confirm.mp3   Confirmação de seleção no menu de configuração

CONFIGURAÇÃO (bomb.json - também nesta pasta):
- ctdn   : Duração em segundos (ex: 600 = 10 minutos)
- mode   : 0=Teclado, 1=Fio único, 2=Ordem de fios, 3=Rede RS485
- code   : Código de 4 dígitos para modo Teclado
- ordr   : Ordem dos fios (ex: 1234 = fios em sequência 1, 2, 3, 4)
- detW   : Fio de detonação (0=nenhum, 1=aleatório, 2-5=fios 1-4)
- dffW   : Fio de desarme (mesmo mapeamento)
- spdW   : Fio de dupla velocidade (0=nenhum, 1=aleatório, 2-5)
- psW    : Fio de pausa temporária
- vol    : Volume (0-30)
- bri    : Brilho do display (0-15)
- tdEt   : Duração do pulso relay de detonação em segundos
- tdEF   : Duração do pulso relay de desarme em segundos
- rs485  : 0=desativado, 1=ativo
- netC   : Número de puzzles na rede para desarme via modo 3
- shak   : 0=nenhum, 1=velocidade dupla, 2=pausa, 3=detonar
- tic    : 0=silencioso, 1=toca tick.mp3 a cada segundo

Certifique-se de que o bomb.json (também nesta pasta) esteja configurado corretamente antes do upload.
