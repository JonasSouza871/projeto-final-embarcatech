#ifndef FUNCOES_GRAFICAS_H
#define FUNCOES_GRAFICAS_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "Display_Bibliotecas/ssd1306.h"
#include "Matriz_Bibliotecas/matriz_led.h"

// Tipos de função
typedef enum {
    FUNCAO_AFIM,
    FUNCAO_QUADRATICA,
    FUNCAO_SENOIDAL,
    FUNCAO_COSSENOIDAL,
    TOTAL_FUNCOES
} TipoFuncao;

// Estados do sistema
typedef enum {
    ESTADO_MENU,
    ESTADO_CONFIGURAR_PARAMETROS,
    ESTADO_EXIBIR_GRAFICO,
    ESTADO_EXIBIR_VALORES
} EstadoSistema;

// Estrutura de dados do sistema
typedef struct {
    ssd1306_t tela;
    EstadoSistema estado_atual;
    TipoFuncao funcao_selecionada;
    float parametros[4];  // A, B, C, D (dependendo da função)
    uint8_t indice_parametro_atual;
    absolute_time_t tempo_ultimo_botao;
    float nivel_zoom;           // Nível de zoom
    float posicao_central_x;    // Posição central do gráfico no eixo X
} Sistema;

// Protótipos das funções
void inicializar_sistema(Sistema *sistema);
void desenhar_tela_menu(Sistema *sistema);
void desenhar_tela_configuracao_parametros(Sistema *sistema);
void plotar_grafico_funcao_afim(Sistema *sistema);
void plotar_grafico_funcao_quadratica(Sistema *sistema);
void plotar_grafico_funcao_senoidal(Sistema *sistema);
void plotar_grafico_funcao_cossenoidal(Sistema *sistema);
void desenhar_tela_valores_quadratica(Sistema *sistema);
void gerenciar_estado_menu(Sistema *sistema);
void gerenciar_estado_grafico(Sistema *sistema);
void atualizar_cores_rgb();
void atualizar_brilho_zoom();

#endif