#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "Display_Bibliotecas/ssd1306.h"
#include "Matriz_Bibliotecas/matriz_led.h"
#include <string.h>
#include <stdlib.h>

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
}

void desenhar_menu() {
    ssd1306_fill(&sistema.display, false);

    const char *titulo = "SELECIONE:";
    ssd1306_draw_string(&sistema.display, titulo, 0, 0);

    const char *funcoes[TOTAL_FUNCOES] = {
        "1. AFIM",
        "2. QUADRATICA",
        "3. SENO",
        "4. COSSENO"
    };

    for(int i = 0; i < TOTAL_FUNCOES; i++) {
        uint8_t y = 16 + i * 12;
        if(i == sistema.funcao_selecionada) {
            ssd1306_draw_string(&sistema.display, ">", 4, y);
            ssd1306_draw_string(&sistema.display, funcoes[i], 16, y);
        } else {
            ssd1306_draw_string(&sistema.display, funcoes[i], 12, y);
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
    ssd1306_draw_string(&sistema.display, buffer, 0, 0);

    // Valor atual do parâmetro selecionado
    snprintf(buffer, sizeof(buffer), "Valor: %.2f", sistema.parametros[sistema.parametro_atual]);
    ssd1306_draw_string(&sistema.display, buffer, 0, 20);

    // Exibir valores de A e B
    snprintf(buffer, sizeof(buffer), "A:%.2f B:%.2f", sistema.parametros[0], sistema.parametros[1]);
    ssd1306_draw_string(&sistema.display, buffer, 0, 40);

    // Instruções
    ssd1306_draw_string(&sistema.display, "BTN: Confirmar", 0, 50);

    ssd1306_send_data(&sistema.display);
}

void plotar_funcao_afim() {
    ssd1306_fill(&sistema.display, false);

    // Desenhar eixos
    ssd1306_vline(&sistema.display, 64, 0, 64, true);  // Eixo Y
    ssd1306_hline(&sistema.display, 0, 127, 32, true); // Eixo X

    // Plotar pontos
    for(int x = -64; x < 64; x++) {
        float y = sistema.parametros[0] * x + sistema.parametros[1];
        int y_pos = 32 - (int)(y / 2);  // Escala de 2 unidades por pixel
        if(y_pos >= 0 && y_pos < 64) {
            ssd1306_pixel(&sistema.display, x + 64, y_pos, true);
        }
    }

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
                break;
        }
        sleep_ms(50);
    }
    return 0;
}
