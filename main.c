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
#define PINO_JOYSTICK_Y 26
#define PINO_BOTAO_JOYSTICK 22
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define ATRASO_DEBOUNCE_MS 500
#define ZONA_MORTA 300

// Estados do sistema
typedef enum {
    ESTADO_MENU,
    ESTADO_CONFIGURAR_PARAMETROS,
    ESTADO_EXIBIR_GRAFICO
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
    ssd1306_t tela;
    EstadoSistema estado_atual;
    TipoFuncao funcao_selecionada;
    float parametros[3];  // A, B, C (dependendo da função)
    uint8_t indice_parametro_atual;
    absolute_time_t tempo_ultimo_botao;
    float nivel_zoom;           // Nível de zoom
    float posicao_central_x;    // Posição central do gráfico no eixo X
} Sistema;

// Variáveis globais
Sistema sistema;

// Protótipos das funções
void inicializar_hardware();
void tratar_interrupcao_gpio(uint gpio, uint32_t events);
void desenhar_tela_menu();
void desenhar_tela_configuracao_parametros();
void plotar_grafico_funcao_afim();
void gerenciar_estado_menu();
void gerenciar_estado_grafico();

void inicializar_hardware() {
    // Configuração I2C
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    // Inicialização do display
    ssd1306_init(&sistema.tela, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&sistema.tela);
    ssd1306_fill(&sistema.tela, false);
    ssd1306_send_data(&sistema.tela);

    // Configuração dos botões
    const uint pinos[] = {PINO_BOTAO_JOYSTICK, PINO_BOTAO_A, PINO_BOTAO_B};
    for (int i = 0; i < 3; i++) {
        gpio_init(pinos[i]);
        gpio_set_dir(pinos[i], GPIO_IN);
        gpio_pull_up(pinos[i]);
        gpio_set_irq_enabled_with_callback(pinos[i], GPIO_IRQ_EDGE_FALL, true, &tratar_interrupcao_gpio);
    }

    // Configuração ADC
    adc_init();
    adc_gpio_init(PINO_JOYSTICK_Y);

    // Inicialização da matriz de LED
    inicializar_matriz_led();

    // Estado inicial
    sistema.estado_atual = ESTADO_MENU;
    sistema.funcao_selecionada = FUNCAO_AFIM;
    sistema.tempo_ultimo_botao = get_absolute_time();
    sistema.nivel_zoom = 1.0;
    sistema.posicao_central_x = 0.0;
}

void tratar_interrupcao_gpio(uint gpio, uint32_t events) {
    absolute_time_t agora = get_absolute_time();

    // Verificação de debounce
    if (absolute_time_diff_us(sistema.tempo_ultimo_botao, agora) < ATRASO_DEBOUNCE_MS * 1000) {
        return;
    }

    sistema.tempo_ultimo_botao = agora;

    if (gpio == PINO_BOTAO_JOYSTICK) {
        if (sistema.estado_atual == ESTADO_MENU) {
            if (sistema.funcao_selecionada == FUNCAO_AFIM) {
                sistema.estado_atual = ESTADO_CONFIGURAR_PARAMETROS;
                sistema.indice_parametro_atual = 0;
                sistema.parametros[0] = 0.0;  // Resetar parâmetros
                sistema.parametros[1] = 0.0;
                desenhar_tela_configuracao_parametros();
            }
        } else if (sistema.estado_atual == ESTADO_CONFIGURAR_PARAMETROS) {
            if (sistema.indice_parametro_atual < 1) {  // Apenas A e B para função afim
                sistema.indice_parametro_atual++;
                desenhar_tela_configuracao_parametros();
            } else {
                sistema.estado_atual = ESTADO_EXIBIR_GRAFICO;
                plotar_grafico_funcao_afim();
            }
        } else if (sistema.estado_atual == ESTADO_EXIBIR_GRAFICO) {
            sistema.estado_atual = ESTADO_MENU;
            desenhar_tela_menu();
        }
    } else if (gpio == PINO_BOTAO_A) {
        if (sistema.estado_atual == ESTADO_CONFIGURAR_PARAMETROS) {
            sistema.parametros[sistema.indice_parametro_atual] += 0.5;
            desenhar_tela_configuracao_parametros();
        }
    } else if (gpio == PINO_BOTAO_B) {
        if (sistema.estado_atual == ESTADO_CONFIGURAR_PARAMETROS) {
            sistema.parametros[sistema.indice_parametro_atual] -= 0.5;
            desenhar_tela_configuracao_parametros();
        }
    }
}

void desenhar_tela_menu() {
    ssd1306_fill(&sistema.tela, false);

    const char *titulo = "SELECIONE:";
    ssd1306_draw_string(&sistema.tela, titulo, 0, 0, false);

    const char *funcoes[TOTAL_FUNCOES] = {
        "1. AFIM",
        "2. QUADRATICA",
        "3. SENO",
        "4. COSSENO"
    };

    for (int i = 0; i < TOTAL_FUNCOES; i++) {
        uint8_t y = 16 + i * 12;
        if (i == sistema.funcao_selecionada) {
            ssd1306_draw_string(&sistema.tela, ">", 4, y, false);
            ssd1306_draw_string(&sistema.tela, funcoes[i], 16, y, false);
        } else {
            ssd1306_draw_string(&sistema.tela, funcoes[i], 12, y, false);
        }
    }
    ssd1306_send_data(&sistema.tela);
}

void desenhar_tela_configuracao_parametros() {
    ssd1306_fill(&sistema.tela, false);

    const char *nomes_parametros[] = {"A", "B"};
    char buffer[20];

    // Título
    snprintf(buffer, sizeof(buffer), "CONFIGURAR %s:", nomes_parametros[sistema.indice_parametro_atual]);
    ssd1306_draw_string(&sistema.tela, buffer, 0, 0, false);

    // Valor atual do parâmetro selecionado
    snprintf(buffer, sizeof(buffer), "Valor: %.2f", sistema.parametros[sistema.indice_parametro_atual]);
    ssd1306_draw_string(&sistema.tela, buffer, 0, 20, false);

    // Exibir valores de A e B
    snprintf(buffer, sizeof(buffer), "A:%.2f B:%.2f", sistema.parametros[0], sistema.parametros[1]);
    ssd1306_draw_string(&sistema.tela, buffer, 0, 40, false);

    // Instruções
    ssd1306_draw_string(&sistema.tela, "BTN: Confirmar", 0, 50, false);

    ssd1306_send_data(&sistema.tela);
}

void plotar_grafico_funcao_afim() {
    // Limpar o display
    ssd1306_fill(&sistema.tela, false);

    // Desenhar os eixos
    ssd1306_vline(&sistema.tela, 64, 0, 63, true);  // Eixo Y
    ssd1306_hline(&sistema.tela, 0, 127, 32, true); // Eixo X

    // Espaçamento dos números nos eixos X e Y
    int espacamento_x = 10;  // Espaçamento entre os números no eixo X
    int espacamento_y = 5;  // Espaçamento entre os números no eixo Y

    // Desenhar números no eixo X
    for (int i = -30; i <= 30; i += espacamento_x) {
        int x_pos = 64 + i / sistema.nivel_zoom;
        if (x_pos >= 0 && x_pos < 128) {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "%d", i);
            ssd1306_draw_string(&sistema.tela, buffer, x_pos - 4, 34, true);
        }
    }

    // Desenhar números no eixo Y
    for (int i = -15; i <= 15; i += espacamento_y) {
        int y_pos = 32 - i / sistema.nivel_zoom;
        if (y_pos >= 0 && y_pos < 64) {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "%d", i);
            ssd1306_draw_string(&sistema.tela, buffer, 66, y_pos - 2, true);
        }
    }

    // Plotar a função afim
    for (int x = -30; x < 30; x++) {
        float x_val = (x - sistema.posicao_central_x) / sistema.nivel_zoom;
        float y = sistema.parametros[0] * x_val + sistema.parametros[1]; // Cálculo da função afim: y = ax + b
        int y_pos = 32 - (int)(y * sistema.nivel_zoom); // Escala do valor de y para o display
        if (y_pos >= 0 && y_pos < 64) { // Garantir que o ponto está dentro dos limites do display
            ssd1306_pixel(&sistema.tela, x + 64, y_pos, true);
        }
    }

    // Enviar os dados para o display
    ssd1306_send_data(&sistema.tela);
}

void gerenciar_estado_menu() {
    adc_select_input(0);
    uint16_t leitura_y = adc_read();
    int16_t diferenca = (int16_t)leitura_y - 2048;

    if (abs(diferenca) > ZONA_MORTA) {
        if (diferenca > 0) {
            mostrar_seta(true);  // Seta para cima
        } else {
            mostrar_seta(false);  // Seta para baixo
        }

        if (diferenca > 0 && sistema.funcao_selecionada > 0) {
            sistema.funcao_selecionada--;
        } else if (diferenca < 0 && sistema.funcao_selecionada < TOTAL_FUNCOES - 1) {
            sistema.funcao_selecionada++;
        }
        desenhar_tela_menu();
        sleep_ms(200);
    } else {
        desligar_matriz();  // Desligar a matriz se o joystick estiver na posição neutra
    }
}

void gerenciar_estado_grafico() {
    adc_select_input(0);
    uint16_t leitura_y = adc_read();
    int16_t diferenca = (int16_t)leitura_y - 2048;

    if (abs(diferenca) > ZONA_MORTA) {
        if (diferenca < 0) {
            sistema.nivel_zoom *= 1.1;  // Aumentar o zoom
        } else {
            sistema.nivel_zoom /= 1.1;  // Diminuir o zoom
        }

        // Limitar o zoom para evitar valores extremos
        if (sistema.nivel_zoom < 0.1) sistema.nivel_zoom = 0.1;
        if (sistema.nivel_zoom > 10.0) sistema.nivel_zoom = 10.0;

        plotar_grafico_funcao_afim();
        sleep_ms(200);
    }
}

int main() {
    stdio_init_all();
    inicializar_hardware();
    desenhar_tela_menu();

    while (true) {
        switch (sistema.estado_atual) {
            case ESTADO_MENU:
                gerenciar_estado_menu();
                break;
            case ESTADO_CONFIGURAR_PARAMETROS:
                // Tratado por interrupções
                break;
            case ESTADO_EXIBIR_GRAFICO:
                gerenciar_estado_grafico();
                break;
        }
        sleep_ms(50);
    }
    return 0;
}
