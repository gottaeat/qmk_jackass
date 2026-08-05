/* Copyright 2024 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "quantum.h"
#include "keychron_common.h"
#include "analog_matrix.h"
#include "profile.h"
#include "eeprom.h"
#include "nvm_eeprom_eeconfig_internal.h"

static const uint8_t profile_actuation_points[PROFILE_COUNT] = {25, 15};

static uint8_t  current_profile_index;
static uint32_t profile_indicator_timer;
static bool     profile_indicator_running;

void profile_init(bool reset) {
    if (reset) {
        current_profile_index = 0;
        analog_matrix_eeprom_update(&current_profile_index, (void *)OFFSET_CURRENT_PROFILE, sizeof(current_profile_index));
        return;
    }

    current_profile_index = eeprom_read_byte((uint8_t *)(EECONFIG_BASE_ANALOG_MATRIX + OFFSET_CURRENT_PROFILE));
    if (current_profile_index >= PROFILE_COUNT) {
        current_profile_index = 0;
    }
}

uint8_t profile_get_actuation_point(void) {
    return profile_actuation_points[current_profile_index];
}

static void profile_select(uint8_t profile_index) {
    if (profile_index != current_profile_index) {
        current_profile_index = profile_index;
        analog_matrix_clear();
        update_travel_configs();

        if (!eeconfig_is_kb_datablock_valid()) {
            eeprom_update_dword(EECONFIG_KEYBOARD, EECONFIG_KB_DATA_VERSION);
        }
        analog_matrix_eeprom_update(&current_profile_index, (void *)OFFSET_CURRENT_PROFILE, sizeof(current_profile_index));
    }

    profile_indicator_timer   = timer_read32();
    profile_indicator_running = true;
    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
}

bool process_record_profile(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case JM_PROF_NEXT:
            profile_select((current_profile_index + 1) % PROFILE_COUNT);
            return false;
        default:
            return true;
    }
}

void profile_indication_timer_check(void) {
    if (profile_indicator_running && timer_elapsed32(profile_indicator_timer) >= 1000) {
        profile_indicator_running = false;
    }
}

bool profile_indication_active(void) {
    return profile_indicator_running;
}

void profile_key_indication(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            keypos_t key = {.row = row, .col = col};

            if (keymap_key_to_keycode(layer_switch_get_layer(key), key) != JM_PROF_NEXT) {
                continue;
            }

            uint8_t led_index = g_led_config.matrix_co[row][col];
            if (led_index != NO_LED) {
                rgb_matrix_set_color(led_index, 255, current_profile_index == 0 ? 0 : 255, 0);
            }
        }
    }
}

void profile_indication(void) {
    if (!profile_indicator_running) {
        return;
    }

    if (current_profile_index == 0) {
        rgb_matrix_set_color_all(255, 0, 0);
    } else {
        rgb_matrix_set_color_all(255, 255, 0);
    }
}
