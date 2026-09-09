/* Copyright 2024 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "keychron.h"

bool dip_switch_update_kb(uint8_t index, bool active) {
    if (index == 0) {
        default_layer_set(1UL << (active ? 2 : 0));
    }
    dip_switch_update_user(index, active);

    return true;
}

void keyboard_post_init_kb(void) {
    bool    backlight_enabled = rgb_matrix_is_enabled();
    uint8_t brightness        = rgb_matrix_get_val();

    keychron_common_init();
    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv_noeeprom(0, 0, brightness);
    if (!backlight_enabled) {
        rgb_matrix_disable_noeeprom();
    }
    keyboard_post_init_user();
}

bool lpm_is_kb_idle(void) {
    return true;
}
