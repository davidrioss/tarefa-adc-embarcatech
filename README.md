# Atividade de ADC - EmbarcaTech

Este projeto consiste em uma atividade prática para consolidar os conceitos de uso de conversores analógico-digitais (ADC) no Raspberry Pi Pico, utilizando um joystick para controlar LEDs RGB e exibir a posição do joystick em um display SSD1306.

## Estrutura do Projeto

O diretório do projeto contém os seguintes arquivos:

- **adc_leds.c**: Código principal do projeto.
- **CMakeLists.txt**: Configuração do sistema de build usando CMake.
- **pico_sdk_import.cmake**: Importação do SDK do Raspberry Pi Pico.
- **lib/**: Pasta contendo bibliotecas para o display SSD1306.
  - **font.h**: Definições de fontes para o display.
  - **ssd1306.c**: Implementação das funções do display SSD1306.
  - **ssd1306.h**: Cabeçalho das funções do display SSD1306.

## Funcionalidades

- **Controle de LEDs RGB**:
  - O joystick controla a intensidade dos LEDs vermelho e azul via PWM.
  - O LED verde é controlado pelo botão do joystick.
  
- **Display SSD1306**:
  - Exibe um quadrado que se move conforme a posição do joystick.
  - O botão do joystick alterna entre bordas simples e duplas no display.

- **Modos de Operação**:
  - O botão A alterna entre 4 modos:
    1. LEDs desligados.
    2. LED vermelho ligado e azul desligado.
    3. LED azul ligado e vermelho desligado.
    4. Ambos os LEDs ligados.

## Compilação

Para compilar o projeto, siga os passos abaixo:

1. Instale a extensão **Raspberry Pi Pico Project** no Visual Studio Code.
2. Abra o diretório do projeto no VS Code.
3. Configure o ambiente de desenvolvimento para o Raspberry Pi Pico.
4. Compile o projeto usando o comando `Build` no VS Code.

## Execução

1. Conecte o Raspberry Pi Pico ao computador via USB.
2. Carregue o arquivo `.uf2` gerado na compilação para o Pico.
3. O programa será executado automaticamente.

## Dependências

- **Raspberry Pi Pico SDK**: Necessário para compilar o projeto.
- **Biblioteca SSD1306**: Incluída no diretório `lib/`.


## Vídeo de demonstração

- O vídeo de demonstração pode ser visto no link:  <https://youtu.be/7FPJWnVuc5M>