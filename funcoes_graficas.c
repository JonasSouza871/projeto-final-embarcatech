#include "funcoes_graficas.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "hardware/adc.h"
#include "Matriz_Bibliotecas/matriz_led.h" // Inclua este cabeçalho

// Definições dos pinos RGB
#define PINO_RGB_VERMELHO 13
#define PINO_RGB_VERDE 11
#define PINO_RGB_AZUL 12

// Variável global (Sistema)
Sistema sistema;

// Definições de hardware
#define PINO_JOYSTICK_Y 26
#define PINO_BOTAO_JOYSTICK 22
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define ATRASO_DEBOUNCE_MS 300
#define ZONA_MORTA 300

void inicializar_sistema(Sistema *sistema_ptr) {
    // Configuração I2C
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    // Inicialização do display
    ssd1306_init(&sistema_ptr->tela, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&sistema_ptr->tela);
    ssd1306_fill(&sistema_ptr->tela, false);
    ssd1306_send_data(&sistema_ptr->tela);

    // Inicialização da matriz de LED (aqui!)
    inicializar_matriz_led();
}

void desenhar_tela_menu(Sistema *sistema) {
    ssd1306_fill(&sistema->tela, false);

    const char *titulo = "SELECIONE:";
    ssd1306_draw_string(&sistema->tela, titulo, 0, 0, false);

    const char *funcoes[TOTAL_FUNCOES] = {
        "1. AFIM",
        "2. QUADRATICA",
        "3. SENOIDAL",
        "4. COSSENO"
    };

    for (int i = 0; i < TOTAL_FUNCOES; i++) {
        uint8_t y = 16 + i * 12;
        if (i == sistema->funcao_selecionada) {
            ssd1306_draw_string(&sistema->tela, ">", 4, y, false);
            ssd1306_draw_string(&sistema->tela, funcoes[i], 16, y, false);
        } else {
            ssd1306_draw_string(&sistema->tela, funcoes[i], 12, y, false);
        }
    }
    ssd1306_send_data(&sistema->tela);
}

void desenhar_tela_configuracao_parametros(Sistema *sistema) {
    ssd1306_fill(&sistema->tela, false);

    const char *nomes_parametros[] = {"A", "B", "C", "D"};
    char buffer[20];

    // Título
    snprintf(buffer, sizeof(buffer), "CONFIGURAR %s:", nomes_parametros[sistema->indice_parametro_atual]);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 0, false);

    // Valor atual do parâmetro selecionado
    snprintf(buffer, sizeof(buffer), "Valor: %.2f", sistema->parametros[sistema->indice_parametro_atual]);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 20, false);

    // Exibir valores de A e B na mesma linha
    snprintf(buffer, sizeof(buffer), "A:%.2f B:%.2f", sistema->parametros[0], sistema->parametros[1]);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 30, false);

    // Exibir valores de C e D embaixo
    snprintf(buffer, sizeof(buffer), "C:%.2f D:%.2f", sistema->parametros[2], sistema->parametros[3]);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 40, false);

    // Instruções
    ssd1306_draw_string(&sistema->tela, "BTN: Confirmar", 0, 50, false);

    ssd1306_send_data(&sistema->tela);
}

void desenhar_tela_valores_quadratica(Sistema *sistema) {
    ssd1306_fill(&sistema->tela, false);

    float a = sistema->parametros[0];
    float b = sistema->parametros[1];
    float c = sistema->parametros[2];

    // Calcular xv, yv e delta
    float delta = b * b - 4 * a * c;
    float xv = -b / (2 * a);
    float yv = -delta / (4 * a);  // Correção da fórmula para yv

    char buffer[30];

    // Exibir xv
    snprintf(buffer, sizeof(buffer), "Xv: %.2f", xv);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 0, false);

    // Exibir yv
    snprintf(buffer, sizeof(buffer), "Yv: %.2f", yv);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 10, false);

    // Exibir delta
    snprintf(buffer, sizeof(buffer), "Delta: %.2f", delta);
    ssd1306_draw_string(&sistema->tela, buffer, 0, 20, false);

    // Instruções
    ssd1306_draw_string(&sistema->tela, "BTN: Voltar", 0, 50, false);

    ssd1306_send_data(&sistema->tela);
}

void plotar_grafico_funcao_afim(Sistema *sistema) {
    // Limpar o display
    ssd1306_fill(&sistema->tela, false);

    // Parâmetros de visualização
    int centro_x = 64;  // Centro do display no eixo X
    int centro_y = 32;  // Centro do display no eixo Y
    float escala_x = sistema->nivel_zoom;  // Escala para o eixo X
    float escala_y = 0.5 * sistema->nivel_zoom;  // Escala para o eixo Y (ajustada para melhor visualização)

    // Desenhar os eixos
    ssd1306_vline(&sistema->tela, centro_x, 0, 63, true);  // Eixo Y
    ssd1306_hline(&sistema->tela, 0, 127, centro_y, true); // Eixo X

    // Configuração dos marcadores nos eixos
    int espacamento_x = 5;  // Espaçamento entre os números no eixo X
    int espacamento_y = 5;  // Espaçamento entre os números no eixo Y

    // Desenhar marcadores no eixo X
    for (int i = -30; i <= 30; i += espacamento_x) {
        int x_pos = centro_x + (int)((i - sistema->posicao_central_x) * escala_x);
        if (x_pos >= 0 && x_pos < 128) {
            // Desenhar marcador
            ssd1306_vline(&sistema->tela, x_pos, centro_y - 2, centro_y + 2, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, x_pos - 4, centro_y + 4, true);
            }
        }
    }

    // Desenhar marcadores no eixo Y
    for (int i = -30; i <= 30; i += espacamento_y) {
        int y_pos = centro_y - (int)(i * escala_y);  // Ajustado para posição central
        if (y_pos >= 0 && y_pos < 64) {
            // Desenhar marcador
            ssd1306_hline(&sistema->tela, centro_x - 2, centro_x + 2, y_pos, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, centro_x + 4, y_pos - 2, true);
            }
        }
    }

    // Plotar a função afim usando mais pontos para suavizar a linha
    float a = sistema->parametros[0];
    float b = sistema->parametros[1];

    int ultimo_y_pos = -1;
    for (int px = 0; px < 128; px++) {
        // Converter coordenada do pixel para coordenada matemática
        float x_val = (px - centro_x) / escala_x + sistema->posicao_central_x;

        // Calcular y = ax + b
        float y_val = a * x_val + b;

        // Converter coordenada matemática para coordenada do pixel
        int y_pos = centro_y - (int)(y_val * escala_y);

        // Verificar se está dentro dos limites do display
        if (y_pos >= 0 && y_pos < 64) {
            ssd1306_pixel(&sistema->tela, px, y_pos, true);

            // Conectar os pontos para uma linha mais suave
            if (ultimo_y_pos != -1 && abs(y_pos - ultimo_y_pos) > 1) {
                int inicio = (ultimo_y_pos < y_pos) ? ultimo_y_pos : y_pos;
                int fim = (ultimo_y_pos < y_pos) ? y_pos : ultimo_y_pos;

                for (int y = inicio; y <= fim; y++) {
                    if (y >= 0 && y < 64) {
                        ssd1306_pixel(&sistema->tela, px - 1, y, true);
                    }
                }
            }

            ultimo_y_pos = y_pos;
        }
    }

    // Informações sobre o zoom (mantido na parte inferior)
    char info_zoom[20];
    snprintf(info_zoom, sizeof(info_zoom), "Zoom: %.1fx", sistema->nivel_zoom);
    ssd1306_draw_string(&sistema->tela, info_zoom, 0, 55, true);

    // Enviar os dados para o display
    ssd1306_send_data(&sistema->tela);
}

void plotar_grafico_funcao_quadratica(Sistema *sistema) {
    // Limpar o display
    ssd1306_fill(&sistema->tela, false);

    // Parâmetros de visualização
    int centro_x = 64;  // Centro do display no eixo X
    int centro_y = 32;  // Centro do display no eixo Y
    float escala_x = sistema->nivel_zoom;  // Escala para o eixo X
    float escala_y = 0.5 * sistema->nivel_zoom;  // Escala para o eixo Y (ajustada para melhor visualização)

    // Desenhar os eixos
    ssd1306_vline(&sistema->tela, centro_x, 0, 63, true);  // Eixo Y
    ssd1306_hline(&sistema->tela, 0, 127, centro_y, true); // Eixo X

    // Configuração dos marcadores nos eixos
    int espacamento_x = 5;  // Espaçamento entre os números no eixo X
    int espacamento_y = 5;  // Espaçamento entre os números no eixo Y

    // Desenhar marcadores no eixo X
    for (int i = -30; i <= 30; i += espacamento_x) {
        int x_pos = centro_x + (int)((i - sistema->posicao_central_x) * escala_x);
        if (x_pos >= 0 && x_pos < 128) {
            // Desenhar marcador
            ssd1306_vline(&sistema->tela, x_pos, centro_y - 2, centro_y + 2, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, x_pos - 4, centro_y + 4, true);
            }
        }
    }

    // Desenhar marcadores no eixo Y
    for (int i = -30; i <= 30; i += espacamento_y) {
        int y_pos = centro_y - (int)(i * escala_y);  // Ajustado para posição central
        if (y_pos >= 0 && y_pos < 64) {
            // Desenhar marcador
            ssd1306_hline(&sistema->tela, centro_x - 2, centro_x + 2, y_pos, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, centro_x + 4, y_pos - 2, true);
            }
        }
    }

    // Plotar a função quadrática usando mais pontos para suavizar a curva
    float a = sistema->parametros[0];
    float b = sistema->parametros[1];
    float c = sistema->parametros[2];

    int ultimo_y_pos = -1;
    for (int px = 0; px < 128; px++) {
        // Converter coordenada do pixel para coordenada matemática
        float x_val = (px - centro_x) / escala_x + sistema->posicao_central_x;

        // Calcular y = ax² + bx + c
        float y_val = a * x_val * x_val + b * x_val + c;

        // Converter coordenada matemática para coordenada do pixel
        int y_pos = centro_y - (int)(y_val * escala_y);

        // Verificar se está dentro dos limites do display
        if (y_pos >= 0 && y_pos < 64) {
            ssd1306_pixel(&sistema->tela, px, y_pos, true);

            // Conectar os pontos para uma curva mais suave (linha vertical)
            if (ultimo_y_pos != -1 && abs(y_pos - ultimo_y_pos) > 1) {
                int inicio = (ultimo_y_pos < y_pos) ? ultimo_y_pos : y_pos;
                int fim = (ultimo_y_pos < y_pos) ? y_pos : ultimo_y_pos;

                for (int y = inicio; y <= fim; y++) {
                    if (y >= 0 && y < 64) {
                        ssd1306_pixel(&sistema->tela, px - 1, y, true);
                    }
                }
            }

            ultimo_y_pos = y_pos;
        }
    }

    // Informações sobre o zoom (mantido na parte inferior)
    char info_zoom[20];
    snprintf(info_zoom, sizeof(info_zoom), "Zoom: %.1fx", sistema->nivel_zoom);
    ssd1306_draw_string(&sistema->tela, info_zoom, 0, 55, true);

    // Enviar os dados para o display
    ssd1306_send_data(&sistema->tela);
}

void plotar_grafico_funcao_senoidal(Sistema *sistema) {
    // Limpar o display
    ssd1306_fill(&sistema->tela, false);

    // Parâmetros de visualização
    int centro_x = 64;  // Centro do display no eixo X
    int centro_y = 32;  // Centro do display no eixo Y
    float escala_x = sistema->nivel_zoom;  // Escala para o eixo X
    float escala_y = 0.5 * sistema->nivel_zoom;  // Escala para o eixo Y (ajustada para melhor visualização)

    // Desenhar os eixos
    ssd1306_vline(&sistema->tela, centro_x, 0, 63, true);  // Eixo Y
    ssd1306_hline(&sistema->tela, 0, 127, centro_y, true); // Eixo X

    // Configuração dos marcadores nos eixos
    int espacamento_x = 5;  // Espaçamento entre os números no eixo X
    int espacamento_y = 5;  // Espaçamento entre os números no eixo Y

    // Desenhar marcadores no eixo X
    for (int i = -30; i <= 30; i += espacamento_x) {
        int x_pos = centro_x + (int)((i - sistema->posicao_central_x) * escala_x);
        if (x_pos >= 0 && x_pos < 128) {
            // Desenhar marcador
            ssd1306_vline(&sistema->tela, x_pos, centro_y - 2, centro_y + 2, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, x_pos - 4, centro_y + 4, true);
            }
        }
    }

    // Desenhar marcadores no eixo Y
    for (int i = -30; i <= 30; i += espacamento_y) {
        int y_pos = centro_y - (int)(i * escala_y);  // Ajustado para posição central
        if (y_pos >= 0 && y_pos < 64) {
            // Desenhar marcador
            ssd1306_hline(&sistema->tela, centro_x - 2, centro_x + 2, y_pos, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, centro_x + 4, y_pos - 2, true);
            }
        }
    }

    // Plotar a função senoidal usando mais pontos para suavizar a curva
    float a = sistema->parametros[0];
    float b = sistema->parametros[1];
    float c = sistema->parametros[2];
    float d = sistema->parametros[3];

    int ultimo_y_pos = -1;
    for (int px = 0; px < 128; px++) {
        // Converter coordenada do pixel para coordenada matemática
        float x_val = (px - centro_x) / escala_x + sistema->posicao_central_x;

        // Calcular y = a + b * sin(c * x + d)
        float y_val = a + b * sin(c * x_val + d);

        // Converter coordenada matemática para coordenada do pixel
        int y_pos = centro_y - (int)(y_val * escala_y);

        // Verificar se está dentro dos limites do display
        if (y_pos >= 0 && y_pos < 64) {
            ssd1306_pixel(&sistema->tela, px, y_pos, true);

            // Conectar os pontos para uma curva mais suave (linha vertical)
            if (ultimo_y_pos != -1 && abs(y_pos - ultimo_y_pos) > 1) {
                int inicio = (ultimo_y_pos < y_pos) ? ultimo_y_pos : y_pos;
                int fim = (ultimo_y_pos < y_pos) ? y_pos : ultimo_y_pos;

                for (int y = inicio; y <= fim; y++) {
                    if (y >= 0 && y < 64) {
                        ssd1306_pixel(&sistema->tela, px - 1, y, true);
                    }
                }
            }

            ultimo_y_pos = y_pos;
        }
    }

    // Informações sobre o zoom (mantido na parte inferior)
    char info_zoom[20];
    snprintf(info_zoom, sizeof(info_zoom), "Zoom: %.1fx", sistema->nivel_zoom);
    ssd1306_draw_string(&sistema->tela, info_zoom, 0, 55, true);

    // Enviar os dados para o display
    ssd1306_send_data(&sistema->tela);
}

void plotar_grafico_funcao_cossenoidal(Sistema *sistema) {
    // Limpar o display
    ssd1306_fill(&sistema->tela, false);

    // Parâmetros de visualização
    int centro_x = 64;  // Centro do display no eixo X
    int centro_y = 32;  // Centro do display no eixo Y
    float escala_x = sistema->nivel_zoom;  // Escala para o eixo X
    float escala_y = 0.5 * sistema->nivel_zoom;  // Escala para o eixo Y (ajustada para melhor visualização)

    // Desenhar os eixos
    ssd1306_vline(&sistema->tela, centro_x, 0, 63, true);  // Eixo Y
    ssd1306_hline(&sistema->tela, 0, 127, centro_y, true); // Eixo X

    // Configuração dos marcadores nos eixos
    int espacamento_x = 5;  // Espaçamento entre os números no eixo X
    int espacamento_y = 5;  // Espaçamento entre os números no eixo Y

    // Desenhar marcadores no eixo X
    for (int i = -30; i <= 30; i += espacamento_x) {
        int x_pos = centro_x + (int)((i - sistema->posicao_central_x) * escala_x);
        if (x_pos >= 0 && x_pos < 128) {
            // Desenhar marcador
            ssd1306_vline(&sistema->tela, x_pos, centro_y - 2, centro_y + 2, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, x_pos - 4, centro_y + 4, true);
            }
        }
    }

    // Desenhar marcadores no eixo Y
    for (int i = -30; i <= 30; i += espacamento_y) {
        int y_pos = centro_y - (int)(i * escala_y);  // Ajustado para posição central
        if (y_pos >= 0 && y_pos < 64) {
            // Desenhar marcador
            ssd1306_hline(&sistema->tela, centro_x - 2, centro_x + 2, y_pos, true);

            // Desenhar número a cada 5 unidades até 10, depois de 10 em 10
            if ((i % 5 == 0 && i <= 10 && i != 0) || (i % 10 == 0 && i > 10)) {
                char buffer[5];
                snprintf(buffer, sizeof(buffer), "%d", i);
                ssd1306_draw_string(&sistema->tela, buffer, centro_x + 4, y_pos - 2, true);
            }
        }
    }

    // Plotar a função cossenoidal usando mais pontos para suavizar a curva
    float a = sistema->parametros[0];
    float b = sistema->parametros[1];
    float c = sistema->parametros[2];
    float d = sistema->parametros[3];

    int ultimo_y_pos = -1;
    for (int px = 0; px < 128; px++) {
        // Converter coordenada do pixel para coordenada matemática
        float x_val = (px - centro_x) / escala_x + sistema->posicao_central_x;

        // Calcular y = a + b * cos(c * x + d)
        float y_val = a + b * cos(c * x_val + d);

        // Converter coordenada matemática para coordenada do pixel
        int y_pos = centro_y - (int)(y_val * escala_y);

        // Verificar se está dentro dos limites do display
        if (y_pos >= 0 && y_pos < 64) {
            ssd1306_pixel(&sistema->tela, px, y_pos, true);

            // Conectar os pontos para uma curva mais suave (linha vertical)
            if (ultimo_y_pos != -1 && abs(y_pos - ultimo_y_pos) > 1) {
                int inicio = (ultimo_y_pos < y_pos) ? ultimo_y_pos : y_pos;
                int fim = (ultimo_y_pos < y_pos) ? y_pos : ultimo_y_pos;

                for (int y = inicio; y <= fim; y++) {
                    if (y >= 0 && y < 64) {
                        ssd1306_pixel(&sistema->tela, px - 1, y, true);
                    }
                }
            }

            ultimo_y_pos = y_pos;
        }
    }

    // Informações sobre o zoom (mantido na parte inferior)
    char info_zoom[20];
    snprintf(info_zoom, sizeof(info_zoom), "Zoom: %.1fx", sistema->nivel_zoom);
    ssd1306_draw_string(&sistema->tela, info_zoom, 0, 55, true);

    // Enviar os dados para o display
    ssd1306_send_data(&sistema->tela);
}

void gerenciar_estado_menu(Sistema *sistema) {
    adc_select_input(0);
    uint16_t leitura_y = adc_read();
    int16_t diferenca = (int16_t)leitura_y - 2048;

    if (abs(diferenca) > ZONA_MORTA) {
        if (diferenca > 0) {
            mostrar_seta(true);  // Seta para cima
        } else {
            mostrar_seta(false);  // Seta para baixo
        }

        if (diferenca > 0 && sistema->funcao_selecionada > 0) {
            sistema->funcao_selecionada--;
        } else if (diferenca < 0 && sistema->funcao_selecionada < TOTAL_FUNCOES - 1) {
            sistema->funcao_selecionada++;
        }
        desenhar_tela_menu(sistema);
        atualizar_cores_rgb();
        sleep_ms(200);
    } else {
        desligar_matriz();  // Desligar a matriz se o joystick estiver na posição neutra
    }
}

void gerenciar_estado_grafico(Sistema *sistema) {
    adc_select_input(0);
    uint16_t leitura_y = adc_read();
    int16_t diferenca = (int16_t)leitura_y - 2048;

    if (abs(diferenca) > ZONA_MORTA) {
        if (diferenca > 0) {  // Joystick para cima
            sistema->nivel_zoom *= 1.1;  // Aumentar o zoom
        } else {  // Joystick para baixo
            sistema->nivel_zoom /= 1.1;  // Diminuir o zoom
        }

        // Limitar o zoom para evitar valores extremos
        if (sistema->nivel_zoom < 0.1) sistema->nivel_zoom = 0.1;
        if (sistema->nivel_zoom > 10.0) sistema->nivel_zoom = 10.0;

        atualizar_brilho_zoom();

        switch (sistema->funcao_selecionada) {
            case FUNCAO_AFIM:
                plotar_grafico_funcao_afim(sistema);
                break;
            case FUNCAO_QUADRATICA:
                plotar_grafico_funcao_quadratica(sistema);
                break;
            case FUNCAO_SENOIDAL:
                plotar_grafico_funcao_senoidal(sistema);
                break;
            case FUNCAO_COSSENOIDAL:
                plotar_grafico_funcao_cossenoidal(sistema);
                break;
        }
        sleep_ms(200);
    }
}

void atualizar_cores_rgb() {
    // Obter o canal correto para cada pino
    uint channel_vermelho = pwm_gpio_to_channel(PINO_RGB_VERMELHO);
    uint channel_verde = pwm_gpio_to_channel(PINO_RGB_VERDE);
    uint channel_azul = pwm_gpio_to_channel(PINO_RGB_AZUL);

    uint slice_vermelho = pwm_gpio_to_slice_num(PINO_RGB_VERMELHO);
    uint slice_verde = pwm_gpio_to_slice_num(PINO_RGB_VERDE);
    uint slice_azul = pwm_gpio_to_slice_num(PINO_RGB_AZUL);

    switch (sistema.funcao_selecionada) {
        case FUNCAO_AFIM:
            pwm_set_chan_level(slice_vermelho, channel_vermelho, 255);
            pwm_set_chan_level(slice_verde, channel_verde, 0);
            pwm_set_chan_level(slice_azul, channel_azul, 0);
            break;
        case FUNCAO_QUADRATICA:
            pwm_set_chan_level(slice_vermelho, channel_vermelho, 0);
            pwm_set_chan_level(slice_verde, channel_verde, 255);
            pwm_set_chan_level(slice_azul, channel_azul, 0);
            break;
        case FUNCAO_SENOIDAL:
            pwm_set_chan_level(slice_vermelho, channel_vermelho, 0);
            pwm_set_chan_level(slice_verde, channel_verde, 0);
            pwm_set_chan_level(slice_azul, channel_azul, 255);
            break;
        case FUNCAO_COSSENOIDAL:
            pwm_set_chan_level(slice_vermelho, channel_vermelho, 255);
            pwm_set_chan_level(slice_verde, channel_verde, 255);
            pwm_set_chan_level(slice_azul, channel_azul, 255);
            break;
    }
}

void atualizar_brilho_zoom() {
    uint channel_vermelho = pwm_gpio_to_channel(PINO_RGB_VERMELHO);
    uint channel_verde = pwm_gpio_to_channel(PINO_RGB_VERDE);
    uint channel_azul = pwm_gpio_to_channel(PINO_RGB_AZUL);

    uint slice_vermelho = pwm_gpio_to_slice_num(PINO_RGB_VERMELHO);
    uint slice_verde = pwm_gpio_to_slice_num(PINO_RGB_VERDE);
    uint slice_azul = pwm_gpio_to_slice_num(PINO_RGB_AZUL);

    uint8_t brilho = (uint8_t)(25.5 * sistema.nivel_zoom);

    switch (sistema.funcao_selecionada) {
        case FUNCAO_AFIM:
            pwm_set_chan_level(slice_vermelho, channel_vermelho, brilho);
            break;
        case FUNCAO_QUADRATICA:
            pwm_set_chan_level(slice_verde, channel_verde, brilho);
            break;
        case FUNCAO_SENOIDAL:
            pwm_set_chan_level(slice_azul, channel_azul, brilho);
            break;
        case FUNCAO_COSSENOIDAL:
            pwm_set_chan_level(slice_vermelho, channel_vermelho, brilho);
            pwm_set_chan_level(slice_verde, channel_verde, brilho);
            pwm_set_chan_level(slice_azul, channel_azul, brilho);
            break;
    }
}
