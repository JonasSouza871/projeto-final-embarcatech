#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "funcoes_graficas.h"  // Inclui o cabeçalho
#include "Matriz_Bibliotecas/matriz_led.h"       // Inclui o cabeçalho da matriz de LED

// Definições de hardware (Movidas para funcoes_graficas.h se forem usadas lá)
#define PINO_JOYSTICK_Y 26
#define PINO_BOTAO_JOYSTICK 22
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define ATRASO_DEBOUNCE_MS 300
#define ZONA_MORTA 300

// Definições dos pinos RGB
#define PINO_RGB_VERMELHO 13
#define PINO_RGB_VERDE 11
#define PINO_RGB_AZUL 12

// Variável global (Sistema) - MOVING para funcoes_graficas.c
extern Sistema sistema;  // Agora é global dentro de funcoes_graficas.c

void tratar_interrupcao_gpio(uint gpio, uint32_t events);

int main() {
    stdio_init_all();

    // Inicialização do sistema
    inicializar_sistema(&sistema);

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
    gpio_set_function(PINO_RGB_VERMELHO, GPIO_FUNC_PWM);
    gpio_set_function(PINO_RGB_VERDE, GPIO_FUNC_PWM);
    gpio_set_function(PINO_RGB_AZUL, GPIO_FUNC_PWM);

    uint slice_vermelho = pwm_gpio_to_slice_num(PINO_RGB_VERMELHO);
    uint slice_verde = pwm_gpio_to_slice_num(PINO_RGB_VERDE);
    uint slice_azul = pwm_gpio_to_slice_num(PINO_RGB_AZUL);

    pwm_set_wrap(slice_vermelho, 255);
    pwm_set_wrap(slice_verde, 255);
    pwm_set_wrap(slice_azul, 255);

    pwm_set_enabled(slice_vermelho, true);
    pwm_set_enabled(slice_verde, true);
    pwm_set_enabled(slice_azul, true);

    // Estado inicial
    sistema.estado_atual = ESTADO_MENU;
    sistema.funcao_selecionada = FUNCAO_AFIM;
    sistema.tempo_ultimo_botao = get_absolute_time();
    sistema.nivel_zoom = 1.0;
    sistema.posicao_central_x = 0.0;

    atualizar_cores_rgb();
    atualizar_brilho_zoom();

    desenhar_tela_menu(&sistema);

    while (true) {
        switch (sistema.estado_atual) {
            case ESTADO_MENU:
                gerenciar_estado_menu(&sistema);
                break;
            case ESTADO_CONFIGURAR_PARAMETROS:
                // Tratado por interrupções
                break;
            case ESTADO_EXIBIR_GRAFICO:
                gerenciar_estado_grafico(&sistema);
                break;
            case ESTADO_EXIBIR_VALORES:
                // Não precisa fazer nada aqui, pois é estático
                break;
        }
        sleep_ms(50);
    }
    return 0;
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
            sistema.estado_atual = ESTADO_CONFIGURAR_PARAMETROS;
            sistema.indice_parametro_atual = 0;
            sistema.parametros[0] = 0.0;  // Resetar parâmetros
            sistema.parametros[1] = 0.0;
            sistema.parametros[2] = 0.0;
            sistema.parametros[3] = 0.0;
            desenhar_tela_configuracao_parametros(&sistema);
        } else if (sistema.estado_atual == ESTADO_CONFIGURAR_PARAMETROS) {
            if (sistema.indice_parametro_atual < 3) {  // A, B, C e D para função cossenoidal
                sistema.indice_parametro_atual++;
                desenhar_tela_configuracao_parametros(&sistema);
            } else {
                sistema.estado_atual = ESTADO_EXIBIR_GRAFICO;
                switch (sistema.funcao_selecionada) {
                  case FUNCAO_AFIM:
                      plotar_grafico_funcao_afim(&sistema);
                      break;
                  case FUNCAO_QUADRATICA:
                      plotar_grafico_funcao_quadratica(&sistema);
                      break;
                  case FUNCAO_SENOIDAL:
                      plotar_grafico_funcao_senoidal(&sistema);
                      break;
                  case FUNCAO_COSSENOIDAL:
                      plotar_grafico_funcao_cossenoidal(&sistema);
                      break;
                }
            }
        } else if (sistema.estado_atual == ESTADO_EXIBIR_GRAFICO) {
            if (sistema.funcao_selecionada == FUNCAO_QUADRATICA) {
                sistema.estado_atual = ESTADO_EXIBIR_VALORES;
                desenhar_tela_valores_quadratica(&sistema);
            } else {
                sistema.estado_atual = ESTADO_MENU;
                desenhar_tela_menu(&sistema);
            }
        } else if (sistema.estado_atual == ESTADO_EXIBIR_VALORES) {
            sistema.estado_atual = ESTADO_MENU;
            desenhar_tela_menu(&sistema);
        }
    } else if (gpio == PINO_BOTAO_A) {
        if (sistema.estado_atual == ESTADO_CONFIGURAR_PARAMETROS) {
            sistema.parametros[sistema.indice_parametro_atual] += 0.5;
            desenhar_tela_configuracao_parametros(&sistema);
        }
    } else if (gpio == PINO_BOTAO_B) {
        if (sistema.estado_atual == ESTADO_CONFIGURAR_PARAMETROS) {
            sistema.parametros[sistema.indice_parametro_atual] -= 0.5;
            desenhar_tela_configuracao_parametros(&sistema);
        }
    }
}