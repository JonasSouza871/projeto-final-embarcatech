#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "Display_Bibliotecas/ssd1306.h"
#include "Matriz_Bibliotecas/matriz_led.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Definições de hardware
#define JOYSTICK_Y_PIN 26
#define JOYSTICK_BUTTON_PIN 22
#define BOTAO_A_PIN 5
#define BOTAO_B_PIN 6
#define DEBOUNCE_DELAY_MS 400
#define ZONA_MORTA 300
#define LONG_PRESS_DURATION_MS 5000

// Estados do sistema
typedef enum {
    ESTADO_MENU,
    ESTADO_PARAMETROS,
    ESTADO_GRAFICO
} EstadoSistema;

// Tipos de função
typedef enum {
    FUNCAO_AFIM,
    FUNCAO_QUADRATICA,
    FUNCAO_SENOIDAL,
    FUNCAO_COSSENOIDAL,
    TOTAL_FUNCOES
} TipoFuncao;

// Estrutura de dados do sistema
typedef struct {
    ssd1306_t display;
    EstadoSistema estado;
    TipoFuncao funcao_selecionada;
    float parametros[3];  // A, B, C (dependendo da função)
    uint8_t parametro_atual;
    absolute_time_t ultimo_tempo_botao;
    absolute_time_t tempo_inicio_long_press;
    bool long_press_detectado;
    float zoom;           // Nível de zoom
    float centro_x;       // Posição central do gráfico no eixo X
} Sistema;

// Variáveis globais
Sistema sistema;

void inicializar_hardware() {
    // Configuração I2C
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    // Inicialização do display
    ssd1306_init(&sistema.display, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&sistema.display);
    ssd1306_fill(&sistema.display, false);
    ssd1306_send_data(&sistema.display);

    // Configuração dos botões
    const uint pins[] = {JOYSTICK_BUTTON_PIN, BOTAO_A_PIN, BOTAO_B_PIN};
    for(int i = 0; i < 3; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    // Configuração ADC
    adc_init();
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Inicialização da matriz de LED
    inicializar_matriz_led();

    // Estado inicial
    sistema.estado = ESTADO_MENU;
    sistema.funcao_selecionada = FUNCAO_AFIM;
    sistema.ultimo_tempo_botao = get_absolute_time();
    sistema.long_press_detectado = false;
    sistema.zoom = 1.0;
    sistema.centro_x = 0.0;
}

void desenhar_menu() {
    ssd1306_fill(&sistema.display, false);

    const char *titulo = "SELECIONE:";
    ssd1306_draw_string(&sistema.display, titulo, 0, 0, false);

    const char *funcoes[TOTAL_FUNCOES] = {
        "1. AFIM",
        "2. QUADRATICA",
        "3. SENO",
        "4. COSSENO"
    };

    for(int i = 0; i < TOTAL_FUNCOES; i++) {
        uint8_t y = 16 + i * 12;
        if(i == sistema.funcao_selecionada) {
            ssd1306_draw_string(&sistema.display, ">", 4, y, false);
            ssd1306_draw_string(&sistema.display, funcoes[i], 16, y, false);
        } else {
            ssd1306_draw_string(&sistema.display, funcoes[i], 12, y, false);
        }
    }
    ssd1306_send_data(&sistema.display);
}

void desenhar_tela_parametros() {
    ssd1306_fill(&sistema.display, false);

    const char *nomes_parametros[] = {"A", "B"};
    char buffer[20];

    // Título
    snprintf(buffer, sizeof(buffer), "CONFIGURAR %s:", nomes_parametros[sistema.parametro_atual]);
    ssd1306_draw_string(&sistema.display, buffer, 0, 0, false);

    // Valor atual do parâmetro selecionado
    snprintf(buffer, sizeof(buffer), "Valor: %.2f", sistema.parametros[sistema.parametro_atual]);
    ssd1306_draw_string(&sistema.display, buffer, 0, 20, false);

    // Exibir valores de A e B
    snprintf(buffer, sizeof(buffer), "A:%.2f B:%.2f", sistema.parametros[0], sistema.parametros[1]);
    ssd1306_draw_string(&sistema.display, buffer, 0, 40, false);

    // Instruções
    ssd1306_draw_string(&sistema.display, "BTN: Confirmar", 0, 50, false);

    ssd1306_send_data(&sistema.display);
}

void plotar_funcao_afim() {
    // Limpar o display
    ssd1306_fill(&sistema.display, false);

    // Desenhar os eixos
    ssd1306_vline(&sistema.display, 64, 0, 63, true);  // Eixo Y
    ssd1306_hline(&sistema.display, 0, 127, 32, true); // Eixo X

    // Escala de espaçamento dos números no eixo X e Y
    int espacamento_x = 20;  // Espaçamento entre os números no eixo X
    int espacamento_y = 10;  // Espaçamento entre os números no eixo Y

    // Desenhar números no eixo X
    for (int i = -60; i <= 60; i += espacamento_x) {
        int x_pos = 64 + i / sistema.zoom;
        if (x_pos >= 0 && x_pos < 128) {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "%d", i);
            ssd1306_draw_string(&sistema.display, buffer, x_pos - 4, 34, true); // Ajuste de posição no eixo X
        }
    }

    // Desenhar números no eixo Y
    for (int i = -30; i <= 30; i += espacamento_y) {
        int y_pos = 32 - i / sistema.zoom;
        if (y_pos >= 0 && y_pos < 64) {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "%d", i);
            ssd1306_draw_string(&sistema.display, buffer, 66, y_pos - 2, true); // Ajuste de posição no eixo Y
        }
    }

    // Plotar a função afim
    for (int x = -64; x < 64; x++) {
        float x_val = (x - sistema.centro_x) / sistema.zoom;
        float y = sistema.parametros[0] * x_val + sistema.parametros[1]; // Cálculo da função afim: y = ax + b
        int y_pos = 32 - (int)(y * sistema.zoom); // Escala do valor de y para o display
        if (y_pos >= 0 && y_pos < 64) { // Garantir que o ponto está dentro dos limites do display
            ssd1306_pixel(&sistema.display, x + 64, y_pos, true);
        }
    }

    // Enviar os dados para o display
    ssd1306_send_data(&sistema.display);
}

void gerenciar_parametros() {
    static absolute_time_t ultimo_tempo_a = 0;
    static absolute_time_t ultimo_tempo_b = 0;

    // Botão A (incremento)
    if(!gpio_get(BOTAO_A_PIN) && absolute_time_diff_us(ultimo_tempo_a, get_absolute_time()) > DEBOUNCE_DELAY_MS * 1000) {
        sistema.parametros[sistema.parametro_atual] += 0.5;
        ultimo_tempo_a = get_absolute_time();
        desenhar_tela_parametros();
    }

    // Botão B (decremento)
    if(!gpio_get(BOTAO_B_PIN) && absolute_time_diff_us(ultimo_tempo_b, get_absolute_time()) > DEBOUNCE_DELAY_MS * 1000) {
        sistema.parametros[sistema.parametro_atual] -= 0.5;
        ultimo_tempo_b = get_absolute_time();
        desenhar_tela_parametros();
    }

    // Botão de confirmação
    if(!gpio_get(JOYSTICK_BUTTON_PIN) && absolute_time_diff_us(sistema.ultimo_tempo_botao, get_absolute_time()) > DEBOUNCE_DELAY_MS * 1000) {
        sistema.ultimo_tempo_botao = get_absolute_time();

        if(sistema.parametro_atual < 1) {  // Apenas A e B para função afim
            sistema.parametro_atual++;
            desenhar_tela_parametros();
        } else {
            sistema.estado = ESTADO_GRAFICO;
            plotar_funcao_afim();
        }
    }

    // Verificação de pressionamento longo
    if(!gpio_get(JOYSTICK_BUTTON_PIN)) {
        if(!sistema.long_press_detectado) {
            sistema.tempo_inicio_long_press = get_absolute_time();
            sistema.long_press_detectado = true;
        } else if(absolute_time_diff_us(sistema.tempo_inicio_long_press, get_absolute_time()) > LONG_PRESS_DURATION_MS * 1000) {
            sistema.estado = ESTADO_MENU;
            desenhar_menu();
        }
    } else {
        sistema.long_press_detectado = false;
    }
}

void gerenciar_menu() {
    adc_select_input(0);
    uint16_t leitura_y = adc_read();
    int16_t diferenca = (int16_t)leitura_y - 2048;

    if(abs(diferenca) > ZONA_MORTA) {
        if(diferenca > 0) {
            mostrar_seta(true);  // Seta para cima
        } else {
            mostrar_seta(false);  // Seta para baixo
        }

        if(diferenca > 0 && sistema.funcao_selecionada > 0) {
            sistema.funcao_selecionada--;
        } else if(diferenca < 0 && sistema.funcao_selecionada < TOTAL_FUNCOES-1) {
            sistema.funcao_selecionada++;
        }
        desenhar_menu();
        sleep_ms(200);
    } else {
        desligar_matriz();  // Desligar a matriz se o joystick estiver na posição neutra
    }

    if(!gpio_get(JOYSTICK_BUTTON_PIN) && absolute_time_diff_us(sistema.ultimo_tempo_botao, get_absolute_time()) > DEBOUNCE_DELAY_MS * 1000) {
        sistema.ultimo_tempo_botao = get_absolute_time();

        if(sistema.funcao_selecionada == FUNCAO_AFIM) {
            sistema.estado = ESTADO_PARAMETROS;
            sistema.parametro_atual = 0;
            sistema.parametros[0] = 0.0;  // Resetar parâmetros
            sistema.parametros[1] = 0.0;
            desenhar_tela_parametros();
        }
    }
}

void gerenciar_grafico() {
    adc_select_input(0);
    uint16_t leitura_y = adc_read();
    int16_t diferenca = (int16_t)leitura_y - 2048;

    if(abs(diferenca) > ZONA_MORTA) {
        if(diferenca < 0) {
            sistema.zoom *= 1.1;  // Aumentar o zoom
        } else {
            sistema.zoom /= 1.1;  // Diminuir o zoom
        }

        // Limitar o zoom para evitar valores extremos
        if(sistema.zoom < 0.1) sistema.zoom = 0.1;
        if(sistema.zoom > 10.0) sistema.zoom = 10.0;

        plotar_funcao_afim();
        sleep_ms(200);
    }

    // Verificação de pressionamento longo
    if(!gpio_get(JOYSTICK_BUTTON_PIN)) {
        if(!sistema.long_press_detectado) {
            sistema.tempo_inicio_long_press = get_absolute_time();
            sistema.long_press_detectado = true;
        } else if(absolute_time_diff_us(sistema.tempo_inicio_long_press, get_absolute_time()) > LONG_PRESS_DURATION_MS * 1000) {
            sistema.estado = ESTADO_MENU;
            desenhar_menu();
        }
    } else {
        sistema.long_press_detectado = false;
    }
}

int main() {
    stdio_init_all();
    inicializar_hardware();
    desenhar_menu();

    while(true) {
        switch(sistema.estado) {
            case ESTADO_MENU:
                gerenciar_menu();
                break;

            case ESTADO_PARAMETROS:
                gerenciar_parametros();
                break;

            case ESTADO_GRAFICO:
                gerenciar_grafico();
                break;
        }
        sleep_ms(50);
    }
    return 0;
}
