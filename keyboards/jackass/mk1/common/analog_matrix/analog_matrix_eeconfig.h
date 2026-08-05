/* Copyright 2024 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#define PROFILE_COUNT 2

#define KC_ANALOG_MATRIX_VERSION 0x4A4D0001
#define SIZE_OF_CALIB_VALUE_T 3

#define OFFSET_CALIBRATION 0
#define OFFSET_CURRENT_PROFILE (OFFSET_CALIBRATION + 1)
#define OFFSET_CALIBRATED_DATA_START (OFFSET_CURRENT_PROFILE + 1)
#define OFFSET_CALIBRATED_DATA_END (OFFSET_CALIBRATED_DATA_START + MATRIX_ROWS * MATRIX_COLS * SIZE_OF_CALIB_VALUE_T)

#define EECONFIG_SIZE_ANALOG_MATRIX OFFSET_CALIBRATED_DATA_END
#define EECONFIG_KB_DATA_VERSION KC_ANALOG_MATRIX_VERSION

#define EXTERNAL_EEPROM_OFFSET 4
