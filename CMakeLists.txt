# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

# Configurações básicas
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configuração do SDK do Raspberry Pi Pico
include(pico_sdk_import.cmake)

# Define o projeto
project(menu_funcoes C CXX ASM)

# Inicializa o SDK do Pico
pico_sdk_init()

# Adiciona os arquivos do programa
add_executable(menu_funcoes
    main.c
    Matriz_Bibliotecas/matriz_led.c
    Display_Bibliotecas/ssd1306.c
)

# Gera o cabeçalho PIO para os LEDs WS2812
# Gerar e incluir o código PIO
pico_generate_pio_header(menu_funcoes ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
target_include_directories(menu_funcoes PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Matriz_Bibliotecas/generated
)

# Habilita comunicação serial
pico_enable_stdio_uart(menu_funcoes 1)
pico_enable_stdio_usb(menu_funcoes 1)  # Ativa comunicação USB

# Vincula as bibliotecas necessárias
target_link_libraries(menu_funcoes PRIVATE
    pico_stdlib      # Biblioteca padrão do Pico
    hardware_i2c     # Suporte para comunicação I2C
    hardware_adc     # Suporte para ADC
    hardware_pio     # Suporte para PIO (necessário para os LEDs WS2812)
    hardware_pwm
)

# Adiciona saídas extras (binário, UF2, etc.)
pico_add_extra_outputs(menu_funcoes)
