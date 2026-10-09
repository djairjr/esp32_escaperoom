#pragma once

#include <M5Cardputer.h>
#include <M5GFX.h>
#include "FiraCode_Regular8pt7b.h"

#define MAX_LINES 200
#define MAX_COLS 48

class ScrollBuffer {
public:
    ScrollBuffer()
        : lineCount_(0), scrollOffset_(0), promptIdx_(-1) {}

    void begin() {
        auto& d = M5Cardputer.Display;
        d.setFont(&FiraCode_Regular8pt7b);
        d.setTextSize(1);
        d.setTextWrap(true, false);
        lineH_ = (int16_t)pgm_read_word(&FiraCode_Regular8pt7b.yAdvance);
        if (lineH_ < 1) lineH_ = 19;
        screenW_ = d.width();
        screenH_ = d.height();
        rows_ = screenH_ / lineH_;
        clear();
    }

    void clear() {
        lineCount_ = 0;
        scrollOffset_ = 0;
        promptIdx_ = -1;
        M5Cardputer.Display.fillScreen(TFT_BLACK);
    }

    void result(const char* t)  { add(t, TFT_GREEN);  }
    void error(const char* t)   { add(t, TFT_YELLOW); }
    void hack(const char* t)    { add(t, TFT_CYAN);   }
    void success(const char* t) { add(t, TFT_GREEN);  }

    void drawPrompt() {
        if (promptIdx_ >= 0) {
            if (strlen(lines_[promptIdx_]) <= 5) strcpy(lines_[promptIdx_], "cmd>");
            colors_[promptIdx_] = TFT_WHITE;
        }
        promptIdx_ = -1;
        if (lineCount_ < MAX_LINES) {
            strcpy(lines_[lineCount_], "cmd> ");
            colors_[lineCount_] = TFT_WHITE;
            lineCount_++;
        } else {
            memmove(lines_[0], lines_[1], (MAX_LINES - 1) * MAX_COLS);
            memmove(colors_, colors_ + 1, (MAX_LINES - 1) * sizeof(uint16_t));
            strcpy(lines_[MAX_LINES - 1], "cmd> ");
            colors_[MAX_LINES - 1] = TFT_WHITE;
        }
        promptIdx_ = lineCount_ - 1;
        scrollOffset_ = 0;
        renderAll();
    }

    void appendChar(char c) {
        if (promptIdx_ < 0) return;
        int cur = strlen(lines_[promptIdx_]);
        lines_[promptIdx_][cur] = c;
        lines_[promptIdx_][cur + 1] = '\0';
        redrawLast();
    }

    void backspace() {
        if (promptIdx_ < 0) return;
        int cur = strlen(lines_[promptIdx_]);
        if (cur <= 5) return;
        lines_[promptIdx_][--cur] = '\0';
        redrawLast();
    }

    const char* getCmdLine() {
        if (promptIdx_ < 0) return "";
        return lines_[promptIdx_] + 5;
    }

    void resetCmdLine() {
        if (promptIdx_ >= 0) {
            strcpy(lines_[promptIdx_], "cmd> ");
            redrawLast();
        }
    }

    void scrollUp() {
        int maxOff = lineCount_ - rows_;
        if (maxOff < 0) maxOff = 0;
        if (scrollOffset_ < maxOff) { scrollOffset_++; renderAll(); }
    }

    void scrollDown() {
        if (scrollOffset_ > 0) { scrollOffset_--; renderAll(); }
    }

private:
    char lines_[MAX_LINES][MAX_COLS];
    uint16_t colors_[MAX_LINES];
    int lineCount_;
    int scrollOffset_;
    int lineH_, screenW_, screenH_, rows_;
    int promptIdx_;

    void add(const char* text, uint16_t color) {
        if (promptIdx_ >= 0) {
            if (strlen(lines_[promptIdx_]) <= 5) strcpy(lines_[promptIdx_], "cmd>");
            colors_[promptIdx_] = TFT_WHITE;
            promptIdx_ = -1;
        }
        appendBuffer(text, color);
    }

    void appendBuffer(const char* text, uint16_t color) {
        if (lineCount_ < MAX_LINES) {
            strlcpy(lines_[lineCount_], text, MAX_COLS);
            colors_[lineCount_] = color;
            lineCount_++;
        } else {
            memmove(lines_[0], lines_[1], (MAX_LINES - 1) * MAX_COLS);
            memmove(colors_, colors_ + 1, (MAX_LINES - 1) * sizeof(uint16_t));
            strlcpy(lines_[MAX_LINES - 1], text, MAX_COLS);
            colors_[MAX_LINES - 1] = color;
            if (promptIdx_ >= 0) promptIdx_--;
        }

        int maxOff = lineCount_ - rows_;
        if (maxOff < 0) maxOff = 0;
        if (scrollOffset_ == 0 || scrollOffset_ > maxOff) scrollOffset_ = 0;

        if (scrollOffset_ == 0 && lineCount_ > rows_) {
            auto& d = M5Cardputer.Display;
            d.copyRect(0, 0, screenW_, screenH_ - lineH_, 0, lineH_);
            d.fillRect(0, screenH_ - lineH_, screenW_, lineH_, TFT_BLACK);
            drawLineAt(lineCount_ - 1, screenH_ - lineH_);
        } else {
            renderAll();
        }
    }

    void drawLineAt(int idx, int y) {
        if (idx < 0 || idx >= lineCount_) return;
        if (y + lineH_ > screenH_) return;
        M5Cardputer.Display.setTextColor(colors_[idx], TFT_BLACK);
        M5Cardputer.Display.setCursor(0, y);
        M5Cardputer.Display.print(lines_[idx]);
    }

    void redrawLast() {
        if (promptIdx_ < 0) { renderAll(); return; }
        int y;
        if (scrollOffset_ == 0 && lineCount_ <= rows_) {
            y = (lineCount_ - 1) * lineH_;
        } else if (scrollOffset_ == 0) {
            y = screenH_ - lineH_;
        } else {
            int end = lineCount_ - 1 - scrollOffset_;
            int start = end - rows_ + 1;
            if (start < 0) start = 0;
            if (end < start) { renderAll(); return; }
            y = (end - start) * lineH_;
        }
        if (y + lineH_ > screenH_) { renderAll(); return; }
        auto& d = M5Cardputer.Display;
        d.fillRect(0, y, screenW_, lineH_, TFT_BLACK);
        drawLineAt(promptIdx_, y);
    }

    void renderAll() {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        int end = lineCount_ - 1 - scrollOffset_;
        int start = end - rows_ + 1;
        if (start < 0) start = 0;
        int y = 0;
        for (int i = start; i <= end && i < lineCount_; i++) {
            if (y + lineH_ > screenH_) break;
            drawLineAt(i, y);
            y += lineH_;
        }
    }
};
