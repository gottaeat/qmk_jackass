/* Copyright 2023 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <assert.h>
#include <stdint.h>
#include "keycodes.h"

enum {
    KC_LOPTN = QK_KB_0,
    KC_ROPTN,
    KC_LCMMD,
    KC_RCMMD,
    KC_MAC_MISSION_CONTROL,
    KC_MAC_LAUCHPAD,
    KC_WIN_TASK_VIEW,
    KC_WIN_FILE_EXPLORER,
    KC_MAC_SCREEN_SHOT,
    KC_WIN_CORTANA,
    KC_MAC_SIRI,
    BT_HST1,
    BT_HST2,
    BT_HST3,
    P2P4G,
    BAT_LVL,
    JM_PROF1,
    JM_PROF2,
    JM_PROF_NEXT,
    NEW_SAFE_RANGE,
};

#define KC_MCTRL KC_MAC_MISSION_CONTROL
#define KC_LNPAD KC_MAC_LAUCHPAD
#define KC_SIRI KC_MAC_SIRI
#define KC_SNAP KC_MAC_SCREEN_SHOT
#define KC_CTANA KC_WIN_CORTANA
#define KC_TASK KC_WIN_TASK_VIEW
#define KC_FILE KC_WIN_FILE_EXPLORER

static_assert((uint16_t)NEW_SAFE_RANGE <= (uint16_t)QK_KB_31, "Keycode overflow");

typedef struct PACKED {
    uint8_t len;
    uint8_t keycode[3];
} key_combination_t;

void keychron_common_init(void);
bool process_record_keychron_common(uint16_t keycode, keyrecord_t *record);
void keychron_common_task(void);
