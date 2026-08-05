/* Copyright 2023~2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "quantum.h"
#include "battery_indicator.h"

#define BAT_LEVEL_DISPLAY_TIME 3000

static bool     active;
static uint8_t  percentage;
static uint32_t display_timer;

void battery_indicator_start(uint8_t value) {
    percentage    = value;
    display_timer = timer_read32();
    active        = true;
}

bool battery_indicator_active(void) {
    return active;
}

void battery_indicator_render(void) {
    uint8_t led_list[] = BAT_LEVEL_LED_LIST;
    uint8_t lit_count  = (percentage + 9) / 10;

    for (uint8_t i = 0; i < sizeof(led_list); i++) {
        rgb_matrix_set_color(led_list[i], i < lit_count ? 255 : 0, i < lit_count ? 255 : 0, i < lit_count ? 255 : 0);
    }
}

static void battery_indicator_update(void) {
    if (active && timer_elapsed32(display_timer) >= BAT_LEVEL_DISPLAY_TIME) {
        active = false;
    }
}

void battery_indicator_task(void) {
    battery_indicator_update();
}
