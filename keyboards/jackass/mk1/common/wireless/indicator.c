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

#define INDEX_MASK 0x0F
#define P24G_IND_MASK 0x10
#define LED_ON 0x80
#define IND_VAL_MASK (P24G_IND_MASK | INDEX_MASK)

#define INDICATOR_SET(state) memcpy(&indicator_config, &state##_config, sizeof(indicator_config_t))

static indicator_config_t pairing_config      = INDICATOR_CONFIG_PARING;
static indicator_config_t connected_config    = INDICATOR_CONFIG_CONNECTD;
static indicator_config_t reconnecting_config = INDICATOR_CONFIG_RECONNECTING;
static indicator_config_t disconnected_config = INDICATOR_CONFIG_DISCONNECTED;
static indicator_config_t indicator_config;
static wt_state_t         indicator_state;
static uint16_t           next_period;
static indicator_type_t   type;
static uint32_t           indicator_timer_buffer;

static uint8_t bt_ind_led_matrix_list[] = BT_INDCATION_LED_MATRIX_LIST;

void indicator_init(void) {
    memset(&indicator_config, 0, sizeof(indicator_config));
}

bool indicator_is_running(void) {
    return !!indicator_config.value;
}

static void indicator_timer_cb(void *arg) {
    if (*(indicator_type_t *)arg != INDICATOR_LAST) type = *(indicator_type_t *)arg;

    bool time_up = false;
    switch (type) {
        case INDICATOR_NONE:
            break;
        case INDICATOR_OFF:
            next_period = 0;
            time_up     = true;
            break;

        case INDICATOR_ON:
            if (indicator_config.value) {
                if (indicator_config.elapsed == 0) {
                    indicator_config.value |= LED_ON;

                    if (indicator_config.duration) {
                        indicator_config.elapsed += indicator_config.duration;
                    }
                } else {
                    time_up = true;
                }
            }
            break;

        case INDICATOR_ON_OFF:
            if (indicator_config.value) {
                if (indicator_config.elapsed == 0) {
                    indicator_config.value |= LED_ON;
                    next_period = indicator_config.on_time;
                } else {
                    indicator_config.value &= IND_VAL_MASK;
                    next_period = indicator_config.duration - indicator_config.on_time;
                }

                if ((indicator_config.duration == 0 || indicator_config.elapsed <= indicator_config.duration) && next_period != 0) {
                    indicator_config.elapsed += next_period;
                } else {
                    time_up = true;
                }
            }
            break;

        case INDICATOR_BLINK:
            if (indicator_config.value) {
                if (indicator_config.value & LED_ON) {
                    indicator_config.value &= IND_VAL_MASK;
                    next_period = indicator_config.off_time;
                } else {
                    indicator_config.value |= LED_ON;
                    next_period = indicator_config.on_time;
                }

                if ((indicator_config.duration == 0 || indicator_config.elapsed <= indicator_config.duration) && next_period != 0) {
                    indicator_config.elapsed += next_period;
                } else {
                    time_up = true;
                }
            }
            break;

        default:
            time_up     = true;
            next_period = 0;
            break;
    }

    if (time_up) {
        indicator_config.value &= IND_VAL_MASK;
        rgb_matrix_indicators_bt();
        indicator_config.value = 0;
        lpm_timer_reset();
    }
}

void indicator_set(wt_state_t state, uint8_t host_index) {
    if (get_transport() == TRANSPORT_USB) return;

    static wt_state_t current_state;
    static uint8_t    current_host;
    bool              host_index_changed = false;

    if (host_index == P24G_HOST_INDEX) host_index = P24G_IND_MASK | 0x01;

    if (current_host != host_index && state != WT_DISCONNECTED) {
        host_index_changed = true;
        current_host       = host_index;
    }

    if (current_state != state || host_index_changed || state == WT_RECONNECTING) {
        /* Some BT chips reset while entering sleep; keep Keychron's ignore rule. */
        if (current_state == WT_SUSPEND && state == WT_DISCONNECTED) return;

        current_state = state;
    } else {
        return;
    }

    indicator_timer_buffer = timer_read32();

    switch (state) {
        case WT_DISCONNECTED:
        case WT_SUSPEND:
            INDICATOR_SET(disconnected);
            indicator_config.value = (indicator_config.type == INDICATOR_NONE) ? 0 : host_index;
            indicator_timer_cb((void *)&indicator_config.type);
            break;

        case WT_CONNECTED:
            if (indicator_state != WT_CONNECTED || host_index_changed) {
                INDICATOR_SET(connected);
                indicator_config.value = (indicator_config.type == INDICATOR_NONE) ? 0 : host_index;
                indicator_timer_cb((void *)&indicator_config.type);
            }
            break;

        case WT_PARING:
            INDICATOR_SET(pairing);
            indicator_config.value = (indicator_config.type == INDICATOR_NONE) ? 0 : LED_ON | host_index;
            indicator_timer_cb((void *)&indicator_config.type);
            break;

        case WT_RECONNECTING:
            INDICATOR_SET(reconnecting);
            indicator_config.value = (indicator_config.type == INDICATOR_NONE) ? 0 : LED_ON | host_index;
            indicator_timer_cb((void *)&indicator_config.type);
            break;

        default:
            break;
    }

    indicator_state = state;
}

void indicator_stop(void) {
    indicator_config.value = 0;
}

void indicator_task(void) {
    battery_indicator_task();

    if (indicator_config.value && timer_elapsed32(indicator_timer_buffer) >= next_period) {
        indicator_timer_cb((void *)&type);
        indicator_timer_buffer = timer_read32();
    }
}

bool rgb_matrix_indicators_bt(void) {
    if (get_transport() == TRANSPORT_USB) return true;

    if (battery_indicator_active()) {
        battery_indicator_render();
        return true;
    }

    if (indicator_config.value) {
        uint8_t host_index = indicator_config.value & INDEX_MASK;

        if (indicator_config.highlight) rgb_matrix_set_color_all(0, 0, 0);

        if (indicator_config.value & LED_ON) {
            uint8_t led_index = (indicator_config.value & P24G_IND_MASK) ? P24G_INDICATION_LED_INDEX : bt_ind_led_matrix_list[host_index - 1];
            rgb_matrix_set_color(led_index, 255, 255, 255);
        }
    }

    return true;
}
