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

#pragma once

#include "wireless.h"

#define P24G_HOST_INDEX 24

/* Keychron K2 HE connection-state indication timings. */
#define INDICATOR_CONFIG_PARING {INDICATOR_BLINK, 1000, 1000, 0, true, 0, 0}
#define INDICATOR_CONFIG_CONNECTD {INDICATOR_ON_OFF, 2000, 250, 2000, true, 0, 0}
#define INDICATOR_CONFIG_RECONNECTING {INDICATOR_BLINK, 100, 100, 600, true, 0, 0}
#define INDICATOR_CONFIG_DISCONNECTED {INDICATOR_NONE, 100, 100, 600, false, 0, 0}

typedef enum {
    INDICATOR_NONE,
    INDICATOR_OFF,
    INDICATOR_ON,
    INDICATOR_ON_OFF,
    INDICATOR_BLINK,
    INDICATOR_LAST,
} indicator_type_t;

typedef struct PACKED {
    indicator_type_t type;
    uint32_t         on_time;
    uint32_t         off_time;
    uint32_t         duration;
    bool             highlight;
    uint8_t          value;
    uint32_t         elapsed;
} indicator_config_t;

void indicator_init(void);
void indicator_set(wt_state_t state, uint8_t host_index);
void indicator_stop(void);
bool indicator_is_running(void);

void indicator_task(void);
bool rgb_matrix_indicators_bt(void);
