/* Copyright 2023 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "quantum.h"
#include "keychron_common.h"
#include "profile.h"
#include "indicator.h"
#include "wireless.h"
#include "transport.h"
#include "lpm.h"

static bool process_record_keychron(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_keychron_common(keycode, record)) return false;
    if (!process_record_profile(keycode, record)) return false;

    if (!process_record_wireless(keycode, record)) return false;

    return true;
}

static bool rgb_matrix_indicators_keychron(void) {
    rgb_matrix_set_color_all(255, 255, 255);

    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(CAPS_LOCK_INDEX, 255, 0, 0);
    }

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
