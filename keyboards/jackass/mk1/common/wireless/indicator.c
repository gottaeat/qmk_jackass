/* Copyright 2023 ~ 2025 @ lokher (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "quantum.h"
#include "indicator.h"
#include "transport.h"
#include "lpm.h"
#include "battery_indicator.h"

#define CONNECTED_HOLD_TIME 2000
#define RECONNECTING_HOLD_TIME 700

static wt_state_t current_state;
static uint8_t    current_host;
static bool       running;
static uint32_t   hold_timer;
static uint32_t   hold_time;

void indicator_init(void) {
    running = false;
}

bool indicator_is_running(void) {
    return running;
}

static void indicator_start(uint32_t duration) {
    hold_timer = timer_read32();
    hold_time  = duration;
    running    = true;
}

void indicator_set(wt_state_t state, uint8_t host_index) {
    if (get_transport() == TRANSPORT_USB) return;

    bool host_changed = current_host != host_index && state != WT_DISCONNECTED;
    if (host_changed) current_host = host_index;

    if (current_state == state && !host_changed && state != WT_RECONNECTING) return;

    /* Some BT chips reset while entering sleep; keep Keychron's ignore rule. */
    if (current_state == WT_SUSPEND && state == WT_DISCONNECTED) return;

    current_state = state;

    switch (state) {
        case WT_CONNECTED:
            indicator_start(CONNECTED_HOLD_TIME);
            break;
        case WT_PARING:
            indicator_start(0);
            break;
        case WT_RECONNECTING:
            indicator_start(RECONNECTING_HOLD_TIME);
            break;
        default:
            indicator_stop();
            break;
    }
}

void indicator_stop(void) {
    running = false;
}

void indicator_task(void) {
    battery_indicator_task();

    if (running && hold_time && timer_elapsed32(hold_timer) >= hold_time) {
        running = false;
        lpm_timer_reset();
    }
}

bool rgb_matrix_indicators_bt(void) {
    if (battery_indicator_active()) {
        battery_indicator_render();
    }
    return true;
}
