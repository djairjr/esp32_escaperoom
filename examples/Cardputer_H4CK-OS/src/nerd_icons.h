#pragma once
/**
 * nerd_icons.h — Ícones Nerd Font para H4CK-OS (Font Awesome 10pt)
 *
 * Fonte: nerd_icons_10.c (ASCII 0x20-0x7E + 25 ícones)
 *
 * Uso:
 *   #include "nerd_icons.h"
 *   scroll.result(ICON_WIFI " Scanning network...");
 */

#include <lvgl.h>

extern const lv_font_t nerd_icons_10;

// ─── Rede / RF ───
#define ICON_WIFI         "\xEF\x87\xAB"  // 
#define ICON_SIGNAL       "\xEF\x80\x92"  // 
#define ICON_GLOBE        "\xEF\x82\xAC"  // 
#define ICON_SATELLITE    "\xEF\x87\xA2"  // 
#define ICON_BROADCAST    "\xEF\x87\x80"  // 

// ─── Segurança ───
#define ICON_LOCK         "\xEF\x80\xA3"  // 
#define ICON_UNLOCK       "\xEF\x82\x9C"  // 
#define ICON_KEY          "\xEF\x82\x84"  // 
#define ICON_SHIELD       "\xEF\x84\xB2"  // 
#define ICON_USER_SECRET  "\xEF\x87\xB8"  // 
#define ICON_SKULL        "\xEF\x94\x99"  // 

// ─── Terminal ───
#define ICON_TERMINAL     "\xEF\x84\xA0"  // 
#define ICON_CODE         "\xEF\x84\xA1"  // 

// ─── Ações ───
#define ICON_SEARCH       "\xEF\x80\x82"  // 

// ─── Status / Alertas ───
#define ICON_WARNING      "\xEF\x81\xB1"  // 
#define ICON_INFO         "\xEF\x81\x9A"  // 
#define ICON_BOLT         "\xEF\x83\xA7"  // 
#define ICON_FIRE         "\xEF\x81\xAD"  // 
#define ICON_TARGET       "\xEF\x81\x9B"  // 
#define ICON_BUG          "\xEF\x86\x88"  // 

// ─── Sistema ───
#define ICON_GEAR         "\xEF\x80\x93"  // 
#define ICON_SERVER       "\xEF\x88\xB3"  // 
#define ICON_EYE          "\xEF\x81\xAE"  // 
#define ICON_HOME         "\xEF\x80\x95"  // 
#define ICON_TRASH        "\xEF\x86\x88"  //  (reusa bug)
