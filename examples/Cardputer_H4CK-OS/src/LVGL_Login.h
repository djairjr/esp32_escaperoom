#pragma once
/**
 * LVGL_Login.h — Tela de login 100% LVGL, 10pt
 *
 * Posicionamento absoluto para evitar problemas com flex layout em tela 240x135.
 */

#include <lvgl.h>
#include "nerd_icons.h"

class LVGL_Login {
public:
    LVGL_Login() : passLen_(0), cursorVisible_(true), lastBlink_(0) {}

    void begin() {
        lv_obj_t* scr = lv_scr_act();
        lv_obj_clean(scr);
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

        // Janela com borda — centralizada na tela 240x135
        cont_ = lv_obj_create(scr);
        lv_obj_set_size(cont_, 210, 100);
        lv_obj_center(cont_);
        lv_obj_set_style_bg_color(cont_, lv_color_make(0x05, 0x05, 0x05), 0);
        lv_obj_set_style_border_color(cont_, lv_color_make(0, 200, 0), 0);
        lv_obj_set_style_border_width(cont_, 1, 0);
        lv_obj_set_style_border_opa(cont_, LV_OPA_40, 0);
        lv_obj_set_style_radius(cont_, 4, 0);
        lv_obj_set_style_pad_all(cont_, 0, 0);
        lv_obj_set_style_pad_top(cont_, 0, 0);

        // Título — centralizado no topo da janela
        titleLabel_ = lv_label_create(cont_);
        lv_label_set_text(titleLabel_, "H4CK-OS v1.0");
        lv_obj_set_style_text_color(titleLabel_, lv_color_make(0, 255, 0), 0);
        lv_obj_set_style_text_font(titleLabel_, &nerd_icons_10, 0);
        lv_obj_align(titleLabel_, LV_ALIGN_TOP_MID, 0, 8);

        // Linha divisória
        line_ = lv_obj_create(cont_);
        lv_obj_set_size(line_, 180, 1);
        lv_obj_set_style_bg_color(line_, lv_color_make(0x30, 0x60, 0x30), 0);
        lv_obj_set_style_border_width(line_, 0, 0);
        lv_obj_align(line_, LV_ALIGN_TOP_MID, 0, 30);

        // "login: root" — alinhado à esquerda abaixo da linha
        loginLabel_ = lv_label_create(cont_);
        lv_label_set_text(loginLabel_, "login: root");
        lv_obj_set_style_text_color(loginLabel_, lv_color_make(0, 255, 255), 0);
        lv_obj_set_style_text_font(loginLabel_, &nerd_icons_10, 0);
        lv_obj_align(loginLabel_, LV_ALIGN_TOP_LEFT, 12, 40);

        // "pass: " + asteriscos — alinhado à esquerda logo abaixo
        passLabel_ = lv_label_create(cont_);
        lv_label_set_text(passLabel_, "pass: ");
        lv_obj_set_style_text_color(passLabel_, lv_color_make(0, 255, 255), 0);
        lv_obj_set_style_text_font(passLabel_, &nerd_icons_10, 0);
        lv_obj_align(passLabel_, LV_ALIGN_TOP_LEFT, 12, 60);

        cursorLabel_ = lv_label_create(cont_);
        lv_label_set_text(cursorLabel_, " ");
        lv_obj_set_style_text_color(cursorLabel_, lv_color_white(), 0);
        lv_obj_set_style_text_font(cursorLabel_, &nerd_icons_10, 0);
        lv_obj_align(cursorLabel_, LV_ALIGN_TOP_LEFT, 12 + 48, 60);  // após "pass: "

        // ACCESS DENIED
        errorLabel_ = lv_label_create(cont_);
        lv_label_set_text(errorLabel_, "ACCESS DENIED");
        lv_obj_set_style_text_color(errorLabel_, lv_color_make(255, 0, 0), 0);
        lv_obj_set_style_text_font(errorLabel_, &nerd_icons_10, 0);
        lv_obj_align(errorLabel_, LV_ALIGN_TOP_LEFT, 12, 82);
        lv_obj_add_flag(errorLabel_, LV_OBJ_FLAG_HIDDEN);

        passLen_ = 0;
        memset(passBuf_, 0, sizeof(passBuf_));
        cursorVisible_ = true;
        lastBlink_ = millis();
        updatePass();
    }

    void clean() {
        if (cont_) { lv_obj_del(cont_); cont_ = nullptr; }
        titleLabel_ = nullptr; loginLabel_ = nullptr;
        passLabel_ = nullptr; cursorLabel_ = nullptr;
        errorLabel_ = nullptr; line_ = nullptr;
    }

    void setError(bool show) {
        if (!errorLabel_) return;
        if (show) lv_obj_remove_flag(errorLabel_, LV_OBJ_FLAG_HIDDEN);
        else      lv_obj_add_flag(errorLabel_, LV_OBJ_FLAG_HIDDEN);
    }

    void appendChar(char c) {
        if (passLen_ < 15) { passBuf_[passLen_++] = c; passBuf_[passLen_] = 0; updatePass(); }
    }

    void backspace() {
        if (passLen_ > 0) { passBuf_[--passLen_] = 0; updatePass(); }
    }

    void reset() {
        passLen_ = 0; memset(passBuf_, 0, sizeof(passBuf_));
        if (cursorLabel_) lv_label_set_text(cursorLabel_, " ");
        setError(false);
    }

    const char* getPassword() const { return passBuf_; }

    void tick() {
        unsigned long now = millis();
        if (now - lastBlink_ > 500) {
            lastBlink_ = now;
            cursorVisible_ = !cursorVisible_;
            updatePass();
        }
    }

private:
    lv_obj_t *cont_ = nullptr, *titleLabel_ = nullptr, *loginLabel_ = nullptr,
             *passLabel_ = nullptr, *cursorLabel_ = nullptr,
             *errorLabel_ = nullptr, *line_ = nullptr;
    char passBuf_[16]; int passLen_;
    bool cursorVisible_; unsigned long lastBlink_;

    void updatePass() {
        if (!cursorLabel_) return;
        if (passLen_ == 0 && !cursorVisible_) {
            lv_label_set_text(cursorLabel_, " ");
            return;
        }
        char buf[18];
        int i;
        for (i = 0; i < passLen_; i++) buf[i] = '*';
        if (cursorVisible_) buf[i++] = '_';
        buf[i] = 0;
        // Se não tem asteriscos nem cursor, põe espaço
        if (i == 0) { buf[0] = ' '; buf[1] = 0; }
        lv_label_set_text(cursorLabel_, buf);
    }
};
