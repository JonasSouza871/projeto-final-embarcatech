#include "ssd1306.h"
#include "font.h"
#include <stdlib.h>
#include "hardware/i2c.h"

// Define commands for SSD1306
#define SET_CONTRAST 0x81
#define SET_ENTIRE_ON 0xA4
#define SET_NORM_INV 0xA6
#define SET_DISP 0xAE
#define SET_MEM_ADDR 0x20
#define SET_COL_ADDR 0x21
#define SET_PAGE_ADDR 0x22
#define SET_DISP_START_LINE 0x40
#define SET_SEG_REMAP 0xA0
#define SET_MUX_RATIO 0xA8
#define SET_COM_OUT_DIR 0xC0
#define SET_DISP_OFFSET 0xD3
#define SET_COM_PIN_CFG 0xDA
#define SET_DISP_CLK_DIV 0xD5
#define SET_PRECHARGE 0xD9
#define SET_VCOM_DESEL 0xDB
#define SET_CHARGE_PUMP 0x8D

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
    ssd->width = width;
    ssd->height = height;
    ssd->pages = height / 8;
    ssd->address = address;
    ssd->i2c_port = i2c;
    ssd->bufsize = ssd->pages * ssd->width + 1;
    ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
    if (ssd->ram_buffer != NULL) {
        ssd->ram_buffer[0] = 0x40; // Co = 0, D/C = 1
    }
    ssd->port_buffer[0] = 0x80; // Co = 1, D/C = 0
}

void ssd1306_config(ssd1306_t *ssd) {
    ssd1306_command(ssd, SET_DISP | 0x00); // Display off
    ssd1306_command(ssd, SET_MEM_ADDR); // Memory mode
    ssd1306_command(ssd, 0x00); // Horizontal addressing mode
    ssd1306_command(ssd, SET_DISP_START_LINE | 0x00); // Start line
    ssd1306_command(ssd, SET_SEG_REMAP | 0x01); // Segment remap
    ssd1306_command(ssd, SET_MUX_RATIO); // Multiplex ratio
    ssd1306_command(ssd, ssd->height - 1); // Height - 1
    ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08); // COM output scan direction
    ssd1306_command(ssd, SET_DISP_OFFSET); // Display offset
    ssd1306_command(ssd, 0x00); // No offset
    ssd1306_command(ssd, SET_COM_PIN_CFG); // COM pin config
    ssd1306_command(ssd, 0x12); // Alternative COM pin config
    ssd1306_command(ssd, SET_DISP_CLK_DIV); // Display clock divide ratio
    ssd1306_command(ssd, 0x80); // Divide ratio
    ssd1306_command(ssd, SET_PRECHARGE); // Pre-charge period
    ssd1306_command(ssd, 0xF1); // Pre-charge value
    ssd1306_command(ssd, SET_VCOM_DESEL); // VCOM deselect level
    ssd1306_command(ssd, 0x30); // VCOM level
    ssd1306_command(ssd, SET_CONTRAST); // Contrast control
    ssd1306_command(ssd, 0xFF); // Contrast value
    ssd1306_command(ssd, SET_ENTIRE_ON); // Entire display on
    ssd1306_command(ssd, SET_NORM_INV); // Normal display
    ssd1306_command(ssd, SET_CHARGE_PUMP); // Charge pump setting
    ssd1306_command(ssd, 0x14); // Enable charge pump
    ssd1306_command(ssd, SET_DISP | 0x01); // Display on
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
    ssd->port_buffer[1] = command;
    i2c_write_blocking(ssd->i2c_port, ssd->address, ssd->port_buffer, 2, false);
}

void ssd1306_send_data(ssd1306_t *ssd) {
    ssd1306_command(ssd, SET_COL_ADDR);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->width - 1);
    ssd1306_command(ssd, SET_PAGE_ADDR);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->pages - 1);
    i2c_write_blocking(ssd->i2c_port, ssd->address, ssd->ram_buffer, ssd->bufsize, false);
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
    uint16_t index = (y / 8) * ssd->width + x + 1;
    uint8_t pixel = y % 8;
    if (value) {
        ssd->ram_buffer[index] |= (1 << pixel);
    } else {
        ssd->ram_buffer[index] &= ~(1 << pixel);
    }
}

void ssd1306_fill(ssd1306_t *ssd, bool value) {
    for (uint8_t y = 0; y < ssd->height; ++y) {
        for (uint8_t x = 0; x < ssd->width; ++x) {
            ssd1306_pixel(ssd, x, y, value);
        }
    }
}

void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill) {
    for (uint8_t x = left; x < left + width; ++x) {
        ssd1306_pixel(ssd, x, top, value);
        ssd1306_pixel(ssd, x, top + height - 1, value);
    }
    for (uint8_t y = top; y < top + height; ++y) {
        ssd1306_pixel(ssd, left, y, value);
        ssd1306_pixel(ssd, left + width - 1, y, value);
    }
    if (fill) {
        for (uint8_t x = left + 1; x < left + width - 1; ++x) {
            for (uint8_t y = top + 1; y < top + height - 1; ++y) {
                ssd1306_pixel(ssd, x, y, value);
            }
        }
    }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        ssd1306_pixel(ssd, x0, y0, value);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value) {
    for (uint8_t x = x0; x <= x1; ++x) {
        ssd1306_pixel(ssd, x, y, value);
    }
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value) {
    for (uint8_t y = y0; y <= y1; ++y) {
        ssd1306_pixel(ssd, x, y, value);
    }
}

void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y) {
    uint16_t index = 0;
    bool rotate = false;
    if (c >= '0' && c <= '9') {
        index = (c - '0' + 1) * 8;
    } else if (c >= 'A' && c <= 'Z') {
        index = (c - 'A' + 11) * 8;
    } else if (c >= 'a' && c <= 'z') {
        index = (c - 'a' + 37) * 8;
    } else if (c == ':') {
        index = 64 * 8; // Index for ':'
        rotate = true;
    } else if (c == '.') {
        index = 65 * 8; // Index for '.'
        rotate = true;
    } else if (c == '>') {
        index = 66 * 8; // Index for '>'
        rotate = true;
    } else if (c == '-') {
        index = 67 * 8; // Index for '-'
        rotate = true;
    }

    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t line = font[index + i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (rotate) {
                // For symbols, apply rotation (90°)
                ssd1306_pixel(ssd, x + (7 - j), y + i, (line >> j) & 0x01);
            } else {
                // For numbers and letters, maintain original position (no rotation)
                ssd1306_pixel(ssd, x + i, y + j, (line >> j) & 0x01);
            }
        }
    }
}

void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y) {
    while (*str) {
        ssd1306_draw_char(ssd, *str, x, y);
        x += 8;
        if (x + 8 >= ssd->width) {
            x = 0;
            y += 8;
        }
        if (y + 8 >= ssd->height) break;
        str++;
    }
}
