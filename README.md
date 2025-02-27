
# 🕹️ Controle de Funções Matemáticas com Joystick e Raspberry Pi Pico W

Este projeto demonstra como visualizar e manipular funções matemáticas (afim, quadrática, senoidal e cossenoidal) usando um joystick e botões em uma Raspberry Pi Pico W. O sistema permite selecionar diferentes funções, configurar seus parâmetros e visualizá-las em um display OLED SSD1306, com feedback visual através de um LED RGB e uma matriz de LEDs WS2812.

---

## 🔧 **Hardware**

*   **Microcontrolador:** Raspberry Pi Pico W
*   **Componentes Principais:**
    *   Display OLED SSD1306 (128x64 pixels)
    *   Joystick analógico
    *   Matriz de LEDs WS2812 (5x5)
    *   LED RGB
    *   Botões de controle (3)
*   **Conexões:**
    *   Cabo Micro-USB para alimentação e programação
    *   Display OLED via I2C (pinos 14 e 15)
    *   Joystick analógico via ADC
    *   Matriz de LEDs via PIO
    *   LED RGB via PWM

### 📍 **Mapeamento de Pinos**

```
# Joystick e Botões
PINO_JOYSTICK_Y: 26 (ADC0)
PINO_BOTAO_JOYSTICK: 22
PINO_BOTAO_A: 5
PINO_BOTAO_B: 6

# LED RGB
PINO_RGB_VERMELHO: 13
PINO_RGB_VERDE: 11
PINO_RGB_AZUL: 12

# Display OLED (I2C)
I2C_SDA: 14
I2C_SCL: 15

# Matriz de LEDs WS2812
PINO_WS2812: 7
```

## 🔌 **Esquemático**

![Esquemático do Projeto](modularizacao-codigo.jpg)

---

## 💻 **Software Necessário**

*   **SDK:** Raspberry Pi Pico SDK (v2.1.0 ou superior)
*   **Ferramentas:** CMake (v3.13+), VS Code (recomendado)
*   **Compilador:** GCC para ARM (fornecido pelo Pico SDK)
*   **Controle de Versão:** Git (opcional, mas recomendado)
*   **Bibliotecas:** 
    * Biblioteca SSD1306 para controle do display OLED
    * Biblioteca WS2812 para controle da matriz de LEDs

---

## 📁 **Estrutura do Projeto**

```
.
├── .vscode/              # Configurações da IDE (VS Code)
├── build/                # Diretório de compilação (gerado pelo CMake)
├── Display_Bibliotecas/  # Bibliotecas para controle do display OLED
│   ├── font.h            # Definições de fontes para o display
│   ├── ssd1306.c         # Implementação do driver SSD1306
│   ├── ssd1306.h         # Interface do driver SSD1306
├── Matriz_Bibliotecas/   # Bibliotecas para controle da matriz de LEDs
│   ├── generated/        # Arquivos gerados pelo PIO
│   ├── matriz_led.c      # Implementação do controle da matriz
│   ├── matriz_led.h      # Interface do controle da matriz
├── .gitignore            # Arquivos ignorados pelo Git
├── CMakeLists.txt        # Configuração do CMake para o build
├── funcoes_graficas.c    # Implementação das funções gráficas
├── funcoes_graficas.h    # Interface das funções gráficas
├── main.c                # Código fonte principal do projeto
├── pico_sdk_import.cmake # Importa o Pico SDK para o CMake
├── diagram.json          # Configuração para simulação no Wokwi
├── wokwi.toml            # Configuração para simulação no Wokwi
```

---

## ⚡ **Como Executar**

### **Clone o Repositório**

```
git clone https://github.com/seu-usuario/grafico-funcoes-pico.git
cd grafico-funcoes-pico
```

### **Compilação (Hardware Físico)**

```
mkdir build
cd build
cmake ..
make
```

### **Gravação no Pico W**

1.  Pressione o botão **BOOTSEL** na sua Pico W e conecte-a ao computador.
2.  A Pico W deve aparecer como um dispositivo de armazenamento USB.
3.  Copie o arquivo `grafico_funcoes.uf2` (gerado na pasta `build/`) para a unidade USB da Pico W.

### **Simulação no Wokwi (Opcional)**

1.  Acesse [Wokwi](https://wokwi.com/).
2.  Crie um novo projeto para Raspberry Pi Pico.
3.  Importe os arquivos `diagram.json` e `wokwi.toml`.
4.  Execute a simulação.

---

## 🚀 **Funcionalidades**

*   Seleção entre quatro tipos de funções matemáticas (afim, quadrática, senoidal e cossenoidal)
*   Configuração dos parâmetros de cada função (A, B, C, D)
*   Visualização gráfica das funções no display OLED
*   Controle de zoom com o joystick
*   Indicação visual da função selecionada através do LED RGB
*   Feedback visual com matriz de LEDs para navegação no menu
*   Exibição de valores específicos para funções quadráticas (vértice e delta)

---

## 📊 **Arquitetura do Código**

1.  **`main.c`:**
    *   **Inicialização:**
        *   Configura GPIO, ADC, PWM para LED RGB
        *   Inicializa o sistema (display, matriz de LEDs)
        *   Configura interrupções para os botões
    *   **Loop Principal (`while(true)`):**
        *   Gerencia os diferentes estados do sistema (menu, configuração, exibição)
        *   Atualiza o display e LEDs conforme necessário
    *   **Funções de Callback:**
        *   `tratarInterrupcaoGPIO`: Gerencia eventos de botões com debounce

2.  **`funcoes_graficas.c`:**
    *   **Funções de Desenho:**
        *   Implementa rotinas para desenhar gráficos das diferentes funções
        *   Gerencia a interface do usuário (menus, configurações)
    *   **Controle de Hardware:**
        *   Controla o LED RGB para indicar a função selecionada
        *   Gerencia o brilho do LED baseado no nível de zoom

3.  **`matriz_led.c`:**
    *   Implementa funções para controlar a matriz de LEDs WS2812
    *   Exibe setas para indicar navegação no menu

4.  **`ssd1306.c`:**
    *   Implementa o driver para o display OLED
    *   Fornece funções para desenhar pixels, linhas, retângulos e texto

---

## 🐛 **Depuração**

*   **Conexões:** Verifique as conexões I2C para o display OLED e as conexões do joystick.
*   **Serial:** Use `printf()` para imprimir valores de variáveis e ajudar a identificar problemas.
*   **Display:** Se o display não mostrar nada, verifique o endereço I2C (0x3C é o padrão).
*   **Joystick:** Certifique-se de que o joystick está calibrado corretamente (valor central em torno de 2048).
*   **Matriz de LEDs:** Verifique a conexão de dados e certifique-se de que a alimentação é suficiente.

---

## 💡 Observações

*   O sistema utiliza uma máquina de estados para gerenciar diferentes modos de operação
*   A biblioteca de fontes inclui caracteres personalizados para melhor visualização no display OLED
*   O controle de zoom é implementado através da escala dos eixos no gráfico
*   A matriz de LEDs WS2812 utiliza a funcionalidade PIO do Raspberry Pi Pico para controle preciso de timing

---

## 🔗 **Vídeo de Funcionamento**

[Link para o vídeo]

## 📞 **Contato**

*   👤 **Autor:** Jonas Souza
*   📧 **E-mail:** jonassouza871@hotmail.com
```

