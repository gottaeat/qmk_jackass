/* Copyright 2024 @ Keychron (https://www.keychron.com)
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

#include <stdint.h>
#include <stdbool.h>
#include "keycodes.h"
#include "matrix.h"
#include "analog_matrix_eeconfig.h"
#include "analog_matrix_type.h"

#define FULL_TRAVEL_UNIT 40

#define DEFAULT_ZERO_TRAVEL_VALUE 3000
#define DEFAULT_FULL_RANGE 900

#define DEFAULT_FULL_TRAVEL_VALUE (DEFAULT_ZERO_TRAVEL_VALUE - DEFAULT_FULL_RANGE)

#define VALID_ANALOG_RAW_VALUE_MIN 1200
#define VALID_ANALOG_RAW_VALUE_MAX 3500
#define ZERO_TRAVEL_DEAD_ZONE 20
#define BOTTOM_JITTER 80

#define TRAVEL_SCALE 6

#define ANALOG_DEBOUCE_TIME 3
#define AUTO_CALIB_FULL_TRAVEL_THRESHOLD_VALUE (DEFAULT_ZERO_TRAVEL_VALUE - DEFAULT_FULL_RANGE + 100)
#define AUTO_CALIB_ZERO_TRAVEL_JITTER_VALUE 50
#define AUTO_CALIB_FULL_TRAVEL_JITTER_VALUE 100
#define AUTO_CALIB_ZERO_TRAVEL_THRESHOLD_VALUE (DEFAULT_FULL_RANGE - AUTO_CALIB_FULL_TRAVEL_JITTER_VALUE)
#define AUTO_CALIB_VALID_RELASING_TIME 1000

void analog_matrix_init(void);
bool update_raw_value(uint8_t row, uint8_t col, uint16_t value);
void update_travel_configs(void);
void analog_matrix_eeprom_update(const void *buf, void *addr, size_t len);

bool analog_matrix_get_key_state(uint8_t row, uint8_t col);
void analog_matrix_task(void);
void analog_matrix_clear(void);
