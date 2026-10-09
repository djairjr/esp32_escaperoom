KeypadTester - Detector de Pinagem para Teclados Matriciais
============================================================

Este exemplo identifica automaticamente quais pinos do PCF8574/PCF8575
estão conectados às LINHAS e COLUNAS de um teclado matricial.

POR QUE USAR?
-------------
Quando você solda fios diretamente no teclado e no módulo PCF, é difícil
saber quais pinos são linhas e quais são colunas. Este sketch resolve isso.

CONEXÕES
--------
1. Conecte 7 pinos do teclado a 7 pinos do PCF857x (quaisquer 7)
2. Para PCF8575: use P00-P06 (pinos 0-6)
3. Para PCF8574: use P0-P6 (pinos 0-6)

CONFIGURAÇÃO NO CÓDIGO
----------------------
Edite estas linhas no início do sketch:

  #define PCF_ADDRESS 0x20      // Endereço I2C do PCF
  #define PCF_IS_8575 true      // true para PCF8575, false para PCF8574

  const byte pinoHardware[7] = {0,1,2,3,4,5,6};  // Pinagem física usada

COMO USAR
---------
1. Carregue o sketch
2. Abra o Serial Monitor (115200 baud)
3. Quando solicitado, pressione e segure cada tecla
4. Solte quando indicado
5. Ao final, o sketch mostra a pinagem rowPins[] e colPins[]

RESULTADO
---------
O sketch exibe código pronto para copiar:

  byte rowPins[4] = {X, X, X, X};
  byte colPins[3] = {X, X, X};

  MatrixKeypadPcf keypad(hw.portExpander(), rowPins, 4, colPins, 3, keymap);

VALIDAÇÃO
---------
Após detectar a pinagem, o sketch entra em modo de teste. Pressione
teclas para validar se estão sendo lidas corretamente.

NOTAS
-----
- Teclado 3x4 = 12 teclas (1-9, *, 0, #)
- Para teclado 4x4, adapte a ordem das teclas no código
- O PCF8575 deve estar no modo 16-bit (todos pinos P00-P15)
