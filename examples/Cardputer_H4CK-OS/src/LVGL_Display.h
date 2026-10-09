#pragma once
/**
 * LVGL_Display.h — Integração LVGL v9 + M5Cardputer
 *
 * Conforme documentação oficial LVGL Arduino:
 * https://github.com/lvgl/lvgl/blob/master/examples/arduino/LVGL_Arduino/LVGL_Arduino.ino
 *
 * E conforme GeminiPuter (exemplo funcional Cardputer + LVGL):
 * https://github.com/nishad2m8/GeminiPuter
 *
 * O teclado NÃO é gerenciado via indev — o Cardputer tem teclado matricial
 * (TCA8418 I2C) que é lido manualmente via M5Cardputer.Keyboard.
 * O LVGL só gerencia o DISPLAY.
 *
 * Uso no setup():
 *   lvgl_init();
 *
 * Uso no loop():
 *   lv_timer_handler();
 *   delay(5);
 */

#include <M5Cardputer.h>
#include <lvgl.h>

// ============================================================================
// DRAW BUFFER — 240 × 10 linhas × 2 bytes = 4800 bytes
// ============================================================================
#define LVGL_HOR_RES 240
#define LVGL_VER_RES 135
#define LVGL_BUF_ROWS 10
#define LVGL_BUF_SIZE (LVGL_HOR_RES * LVGL_BUF_ROWS * 2)

static uint8_t lvgl_buf_[LVGL_BUF_SIZE];

// ============================================================================
// TICK SOURCE
// ============================================================================
static uint32_t lvgl_tick_cb() {
    return millis();
}

// ============================================================================
// DISPLAY FLUSH CALLBACK (igual GeminiPuter)
// ============================================================================
static void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    M5Cardputer.Display.startWrite();
    M5Cardputer.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5Cardputer.Display.pushPixels((uint16_t*)px_map, w * h, true);
    M5Cardputer.Display.endWrite();
    lv_display_flush_ready(disp);
}

// ============================================================================
// INICIALIZAÇÃO
// ============================================================================
void lvgl_init() {
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    // Display (sem indev de teclado — teclado lido manualmente)
    lv_display_t* disp = lv_display_create(LVGL_HOR_RES, LVGL_VER_RES);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, lvgl_buf_, NULL, LVGL_BUF_SIZE,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}
