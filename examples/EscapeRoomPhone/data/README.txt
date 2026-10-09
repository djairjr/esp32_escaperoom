Este exemplo de telefone utiliza um sistema híbrido de armazenamento:

Arquivos na Memória LittleFS (Efeitos de sistema):
- dialtone.mp3   (Tom de discagem)
- busy.mp3       (Sinal de ocupado)
- naoexiste.mp3  (Aviso de número inválido)

Arquivos no Cartão SD (Conteúdo dinâmico):
- [numero].mp3   (Arquivos de áudio para cada número discado, ex: 123.mp3)

Dica: No LittleFS, os arquivos devem estar na raiz (sem a pasta sfx/).
