/* Copyright 2023 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "quantum.h"
#include "eeprom.h"
#include "nvm_eeprom_eeconfig_internal.h"
#include <lib/lib8tion/lib8tion.h>
#include "keychron_common.h"
#include "profile.h"
#include "indicator.h"
#include "wireless.h"
#include "transport.h"
#include "lpm.h"

static bool process_record_keychron_backlight(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case UG_TOGG:
            rgb_matrix_toggle();
            return false;

        case UG_VALU:
            if (!rgb_matrix_config.enable) {
                rgb_matrix_toggle();
                return false;
            }

            rgb_matrix_config.hsv.v = qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
            while (rgb_matrix_config.hsv.v <= RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL) {
                rgb_matrix_config.hsv.v = qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
            }
            eeprom_write_byte((uint8_t *)EECONFIG_RGB_MATRIX + offsetof(rgb_config_t, hsv.v), rgb_matrix_config.hsv.v);
            return false;

        case UG_VALD:
            if (rgb_matrix_config.enable && rgb_matrix_config.hsv.v > RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL) {
                rgb_matrix_config.hsv.v = qsub8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
                eeprom_write_byte((uint8_t *)EECONFIG_RGB_MATRIX + offsetof(rgb_config_t, hsv.v), rgb_matrix_config.hsv.v);
            }
            if (rgb_matrix_config.enable && rgb_matrix_config.hsv.v <= RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL) {
                rgb_matrix_toggle();
            }
            return false;

        default:
            return true;
    }
}

static bool process_record_keychron(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_keychron_common(keycode, record)) return false;
    if (!process_record_profile(keycode, record)) return false;
    if (!process_record_keychron_backlight(keycode, record)) return false;

    if (!process_record_wireless(keycode, record)) return false;

    return true;
}

static void backlight_key_indication(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            keypos_t key = {.row = row, .col = col};

            if (keymap_key_to_keycode(layer_switch_get_layer(key), key) != UG_TOGG) {
                continue;
            }

            uint8_t led_index = g_led_config.matrix_co[row][col];
            if (led_index != NO_LED) {
                rgb_matrix_set_color(led_index, 0, 0, 255);
            }
        }
    }
}

static bool rgb_matrix_indicators_keychron(void) {
    uint8_t brightness = rgb_matrix_get_val();
    rgb_matrix_set_color_all(brightness, brightness, brightness);

    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(CAPS_LOCK_INDEX, 255, 0, 0);
    }

    profile_key_indication();
    backlight_key_indication();
    rgb_matrix_indicators_bt();
    profile_indication();

    /* Cable mode has no backlight until USB power is present. */
    if (get_transport() == TRANSPORT_USB && !usb_power_connected()) {
        rgb_matrix_set_color_all(0, 0, 0);
    }

    return true;
}

static void keychron_task(void) {
    (void)wireless_tasks();
    keychron_common_task();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (IS_RGB_MATRIX_KEYCODE(keycode)) return false;
    if (!process_record_user(keycode, record)) return false;
    return process_record_keychron(keycode, record);
}

bool rgb_matrix_indicators_kb(void) {
    if (!rgb_matrix_indicators_user()) return false;
    return rgb_matrix_indicators_keychron();
}

void housekeeping_task_kb(void) {
    keychron_task();
}
