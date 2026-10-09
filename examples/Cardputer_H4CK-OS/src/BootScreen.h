#pragma once

#include <M5Cardputer.h>
#include <AnimatedGIF.h>
#include "boot_animation_gif.h"

// ============================================================================
// BOOT SCREEN — Animação GIF embutida no firmware
// ============================================================================
// GIF está encapsulado no PROGMEM — sem depender de LittleFS.
// Usa GIF_DRAW_RAW (sem framebuffer extra) → seguro para Cardputer.
// Fallback textual se algo falhar.
// ============================================================================

static void GIFDraw(GIFDRAW* pDraw) {
    uint8_t* s = pDraw->pPixels;
    if (!s) return;

    int y = pDraw->iY + pDraw->y;
    if (y >= M5Cardputer.Display.height()) return;

    int w = pDraw->iWidth;
    if (pDraw->iX + w > M5Cardputer.Display.width())
        w = M5Cardputer.Display.width() - pDraw->iX;
    if (w < 1) return;

    uint16_t* pal = pDraw->pPalette;
    uint8_t t = pDraw->ucTransparent;
    bool hasT = pDraw->ucHasTransparency;

    uint16_t lineBuf[240];
    for (int x = 0; x < w; x++) {
        uint8_t c = s[x];
        lineBuf[x] = (hasT && c == t) ? TFT_BLACK : pal[c];
    }

    M5Cardputer.Display.pushImage(pDraw->iX, y, w, 1, lineBuf);
}

class BootScreen {
public:
    BootScreen() : gif_() {}

    bool begin() {
        gif_.begin(GIF_PALETTE_RGB565_LE);
        gif_.setDrawType(GIF_DRAW_RAW);

        int ret = gif_.open((uint8_t*)boot_animation_gif,
                            boot_animation_gif_len, GIFDraw);
        if (ret != 0) {
            Serial.printf("[BOOT] GIF open error: %d\n", ret);
            return false;
        }

        GIFINFO info;
        gif_.getInfo(&info);
        Serial.printf("[BOOT] GIF OK: %d frames, %dms, %d bytes\n",
            info.iFrameCount, info.iDuration, boot_animation_gif_len);
        return true;
    }

    void play() {
        if (!gif_.getCanvasWidth()) {
            // Fallback textual
            M5Cardputer.Display.fillScreen(TFT_BLACK);
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(30, 55);
            M5Cardputer.Display.print("H4CK-OS");
            M5Cardputer.Display.setTextSize(1);
            delay(2000);
            return;
        }

        while (true) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                M5Cardputer.Keyboard.updateKeysState();
                break;
            }
            if (!gif_.playFrame(false, nullptr)) break;
            delay(1);
        }

        gif_.close();
        // Reabre para próximo boot
        gif_.begin(GIF_PALETTE_RGB565_LE);
        gif_.setDrawType(GIF_DRAW_RAW);
        gif_.open((uint8_t*)boot_animation_gif, boot_animation_gif_len, GIFDraw);
    }

private:
    AnimatedGIF gif_;
};
