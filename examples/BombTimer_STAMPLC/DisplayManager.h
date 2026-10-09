#pragma once

#include <M5GFX.h>
#include "DSEG14Classic_Regular20pt7b.h"

// ============================================================================
// DISPLAY MANAGER — TFT ST7789v2
// Native resolution: 240×135 (ST7789v2)
// Rotation 2 = landscape, USB on the left → w=240, h=135
//
// Layout (w=240, h=135):
//   ┌──────────────────────────────────────┐
//   │ [W][M][R][T]                         │  0-12  ← status bar (top right)
//   │                                      │
//   │         ┌─────────────┐              │
//   │         │   03:00     │              │ 24-72  ← DSEG14 timer (centered)
//   │         └─────────────┘              │
//   │             180s                     │ 78-90  ← small seconds
//   │                                      │
//   │ ●Y ●R ●B ●G        IDLE             │ 115-135 ← circles + status
//   └──────────────────────────────────────┘
// ============================================================================

class DisplayManager {
public:
    void begin() {
        M5.Lcd.begin();
        M5.Lcd.setRotation(1);  // 240×135, USB on the left
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        w_ = M5.Lcd.width();   // 240
        h_ = M5.Lcd.height();  // 135

        // DSEG14 20pt font height (discovered empirically)
        dsegHeight_ = 42;
    }

    int width() { return w_; }
    int height() { return h_; }

    // ─── Status Bar (top right corner) ───────────────────────────────
    // Each "[X]" = 3 chars * ~5px ≈ 15px. 4 icons = 60px.
    // Positions: [W] [M] [R] [T] spaced every 14px from the right
    // Shifted 10px to the left to avoid wrapping the last icon
    void drawWiFiIcon(bool connected) {
        M5.Lcd.setTextColor(connected ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setCursor(w_ - 70, 2);
        M5.Lcd.printf(connected ? "[W]" : "[w]");
    }

    void drawMQTTIcon(bool connected) {
        M5.Lcd.setTextColor(connected ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setCursor(w_ - 55, 2);
        M5.Lcd.printf(connected ? "[M]" : "[m]");
    }

    void drawRFIDIcon(bool detected) {
        M5.Lcd.setTextColor(detected ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setCursor(w_ - 40, 2);
        M5.Lcd.printf(detected ? "[R]" : "[r]");
    }

    void drawRTCIcon(bool valid) {
        M5.Lcd.setTextColor(valid ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setCursor(w_ - 25, 2);
        M5.Lcd.printf(valid ? "[T]" : "[t]");
    }

    // ─── Giant Timer (DSEG14, center) ────────────────────────────────
    // 5 digits + ':': ~128px. Area: y=34 to y=82 (48px free height)
    // DSEG14 20pt font ≈ 42px tall.
    // Vertically centered: y = 34 + (48 - 42) / 2 = 37 ≈ 38
    void drawTimer(int minutes, int seconds) {
        // Clean area: y=34 to y=84 (50px)
        M5.Lcd.fillRect(0, TIMER_Y, w_, 52, TFT_BLACK);

        M5.Lcd.setFont(&DSEG14Classic_Regular20pt7b);
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);

        // Builds the "MM:SS" string and prints it at once
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);

        // Approximate width: 5 chars * ~24px + ':' gap = ~128px
        int textW = 24 * 5 + 8;
        int startX = (w_ - textW) / 2;
        if (startX < 0) startX = 0;

        // y = top + padding to center the 42px font in 50px
        M5.Lcd.setCursor(startX, TIMER_Y + 4);
        M5.Lcd.print(buf);

        M5.Lcd.setFont(&fonts::Font0);
    }

    // ─── Seconds (small, below the timer) ─────────────────────────────
    void drawSecondsSmall(int totalSeconds) {
        M5.Lcd.fillRect(0, SEC_Y, w_, 14, TFT_BLACK);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setCursor((w_ - 40) / 2, SEC_Y + 2);
        M5.Lcd.printf("%ds", totalSeconds);
    }

    // ─── Wire circles (bottom left corner) ───────────────────────────
    static constexpr uint16_t WIRE_COLORS[4] = {
        TFT_YELLOW, TFT_RED, TFT_BLUE, TFT_GREEN
    };

    void drawWireCircle(int idx, bool isCut) {
        if (idx < 0 || idx > 3) return;
        int x = 8 + idx * 18;
        int y = h_ - 12;
        if (isCut) { // Better this way. When connected, filled circle. When disconnected, empty.
            M5.Lcd.fillCircle(x, y, 5, TFT_BLACK);
            M5.Lcd.drawCircle(x, y, 5, WIRE_COLORS[idx]);
        } else {
            M5.Lcd.fillCircle(x, y, 5, WIRE_COLORS[idx]);
        }
    }

    void drawAllWireCircles(const bool cut[4]) {
        for (int i = 0; i < 4; i++) drawWireCircle(i, cut[i]);
    }

    // ─── Status text (bottom right corner) ───────────────────────────
    void drawBottomRight(const char* text, uint16_t color = TFT_WHITE) {
        M5.Lcd.fillRect(w_ / 2, h_ - 20, w_ / 2, 20, TFT_BLACK);
        M5.Lcd.setTextColor(color, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setCursor(w_ / 2 + 4, h_ - 16);
        M5.Lcd.print(text);
    }

    // ─── Full screen (detonation/defusal) ─────────────────────────────
    void drawFullScreenMsg(const char* line1, const char* line2, uint16_t color) {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.setTextColor(color, TFT_BLACK);
        M5.Lcd.setFont(&fonts::Font2);
        M5.Lcd.setTextSize(2);
        int y1 = h_ / 2 - 18;
        int x1 = (w_ - strlen(line1) * 12) / 2;
        if (x1 < 0) x1 = 0;
        M5.Lcd.setCursor(x1, y1);
        M5.Lcd.print(line1);
        if (line2) {
            M5.Lcd.setTextSize(1);
            int x2 = (w_ - strlen(line2) * 6) / 2;
            if (x2 < 0) x2 = 0;
            M5.Lcd.setCursor(x2, h_ / 2 + 12);
            M5.Lcd.print(line2);
        }
    }

    // ─── Defuse code (CODE mode, DSEG14) ─────────────────────────────
    void drawCodeEntry(const char digits[4], int currentDigit) {
        M5.Lcd.fillRect(0, TIMER_Y, w_, 52, TFT_BLACK);
        M5.Lcd.setFont(&DSEG14Classic_Regular20pt7b);
        M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);

        int totalW = 24 * 4 + 12;
        int startX = (w_ - totalW) / 2;
        if (startX < 0) startX = 0;

        // Digit Y (same as the timer)
        int digitY = TIMER_Y + 4;
        // Cursor Y (horizontal dash at the base of the digit)
        int cursorY = digitY + 36;

        for (int i = 0; i < 4; i++) {
            int x = startX + i * (24 + 4);
            if (i < currentDigit && digits[i] != 0) {
                M5.Lcd.setCursor(x, digitY);
                M5.Lcd.print(digits[i]);
            } else if (i == currentDigit) {
                if ((millis() / 500) % 2 == 0)
                    M5.Lcd.fillRect(x + 2, cursorY, 20, 4, TFT_ORANGE);
                else
                    M5.Lcd.fillRect(x + 2, cursorY, 20, 4, TFT_BLACK);
            }
        }
        M5.Lcd.setFont(&fonts::Font0);
    }

    // ─── Selective clearing ──────────────────────────────────────────
    void clearScreen() { M5.Lcd.fillScreen(TFT_BLACK); }
    void clearTimerArea() { M5.Lcd.fillRect(0, TIMER_Y, w_, 54, TFT_BLACK); }
    void clearStatusBar() { M5.Lcd.fillRect(w_ - 80, 0, 80, 14, TFT_BLACK); }
    void clearBottomBar() { M5.Lcd.fillRect(0, h_ - 22, w_, 22, TFT_BLACK); }

private:
    int w_ = 240;
    int h_ = 135;
    int dsegHeight_ = 42;

    static constexpr int TIMER_Y = 34;  // start of the timer area (+10 from the original height)
    static constexpr int SEC_Y   = 88;  // start of the seconds area
};
