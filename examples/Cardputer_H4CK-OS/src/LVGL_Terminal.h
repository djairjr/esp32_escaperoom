#pragma once
/**
 * LVGL_Terminal.h — LVGL terminal with manual input (original ScrollBuffer)
 *
 * Inspired by GeminiPuter: LVGL only renders, keyboard read manually.
 * The input (typing) lives in a C++ buffer and is shown in an LVGL label.
 * The output uses LVGL labels with native scroll.
 *
 * API compatible with the original ScrollBuffer:
 *   result(), error(), hack(), success()
 *   drawPrompt(), appendChar(), backspace(), getCmdLine(), clear()
 *   scrollUp(), scrollDown()
 */

#include <M5Cardputer.h>
#include <lvgl.h>
#include "nerd_icons.h"

#define MAX_LINES 200
#define MAX_COLS 48
#define PROMPT_STR "cmd>"

class LVGL_Terminal {
public:
    LVGL_Terminal() : lineCount_(0) {}

    void begin() {
        lv_obj_t* scr = lv_scr_act();
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

        // ─── Output container (scrollable) ───
        outCont_ = lv_obj_create(scr);
        lv_obj_set_size(outCont_, 240, 118);
        lv_obj_set_pos(outCont_, 0, 0);
        lv_obj_set_style_bg_color(outCont_, lv_color_black(), 0);
        lv_obj_set_style_border_width(outCont_, 0, 0);
        lv_obj_set_style_pad_all(outCont_, 2, 0);
        lv_obj_set_scrollbar_mode(outCont_, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(outCont_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(outCont_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        // ─── Input label (prompt + typing) ───
        inputLabel_ = lv_label_create(scr);
        lv_obj_set_pos(inputLabel_, 2, 120);
        lv_obj_set_style_text_color(inputLabel_, lv_color_white(), 0);
        lv_obj_set_style_bg_color(inputLabel_, lv_color_black(), 0);
        lv_label_set_text(inputLabel_, "cmd> ");

        clear();
    }

    // ─── Output API ───
    void result(const char* t)  { addLine(t, lv_color_make(0, 255, 0)); }
    void error(const char* t)   { addLine(t, lv_color_make(255, 255, 0)); }
    void hack(const char* t)    { addLine(t, lv_color_make(0, 255, 255)); }
    void success(const char* t) { addLine(t, lv_color_make(0, 255, 0)); }

    // ─── Prompt ───
    void drawPrompt() {
        promptLine_[0] = '\0';
        updateInputLabel();
    }

    void appendChar(char c) {
        int cur = strlen(promptLine_);
        if (cur < MAX_COLS - 2) {
            promptLine_[cur] = c;
            promptLine_[cur + 1] = '\0';
            updateInputLabel();
        }
    }

    void backspace() {
        int cur = strlen(promptLine_);
        if (cur > 0) {
            promptLine_[--cur] = '\0';
            updateInputLabel();
        }
    }

    const char* getCmdLine() {
        return promptLine_;
    }

    void resetCmdLine() {
        promptLine_[0] = '\0';
        updateInputLabel();
    }

    // ─── Scroll ───
    void scrollUp() {
        lv_obj_scroll_by(outCont_, 0, -12, LV_ANIM_OFF);
    }

    void scrollDown() {
        lv_obj_scroll_by(outCont_, 0, 12, LV_ANIM_OFF);
    }

    // ─── Clear ───
    void clear() {
        lv_obj_clean(outCont_);
        lineCount_ = 0;
    }

    void clean() {
        outCont_ = nullptr;
        inputLabel_ = nullptr;
    }

private:
    lv_obj_t* outCont_;
    lv_obj_t* inputLabel_;
    char promptLine_[MAX_COLS];
    int lineCount_;

    void updateInputLabel() {
        char buf[MAX_COLS + 8];
        snprintf(buf, sizeof(buf), "%s %s", PROMPT_STR, promptLine_);
        lv_label_set_text(inputLabel_, buf);
    }

    void addLine(const char* text, lv_color_t color) {
        if (lineCount_ >= MAX_LINES) {
            // Remove the first line
            lv_obj_t* first = lv_obj_get_child(outCont_, 0);
            if (first) lv_obj_del(first);
            lineCount_--;
        }

        lineCount_++;

        // Add LVGL label
        lv_obj_t* label = lv_label_create(outCont_);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, color, 0);
        lv_obj_set_width(label, 230);
        lv_obj_set_style_text_font(label, &nerd_icons_10, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        // Scroll to the end
        lv_obj_scroll_to_y(outCont_, 32767, LV_ANIM_OFF);
    }
};
