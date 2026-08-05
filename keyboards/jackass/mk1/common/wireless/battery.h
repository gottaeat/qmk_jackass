/* Copyright 2022 ~ 2025 @ lokher (https://www.keychron.com)
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
#include "config.h"

enum {
    BAT_CHARGING     = 1,
    BAT_FULL_CHARGED = 2,
};

#define FULL_VOLTAGE_VALUE 4100
#define EMPTY_VOLTAGE_VALUE 3500
#define SHUTDOWN_VOLTAGE_VALUE 3300
#define VOLTAGE_MEASURE_INTERVAL 3000
#define VOLTAGE_POWER_ON_MEASURE_COUNT 15
#define BACKLIGHT_OFF_VOLTAGE_MEASURE_INTERVAL 200

void battery_init(void);

void    battery_calculate_voltage(uint16_t value);
uint8_t battery_get_percentage(void);
bool    battery_is_critical_low(void);
void    battery_timer_reset(void);
void    battery_task(void);
