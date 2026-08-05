/* Copyright 2024 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "analog_matrix_eeconfig.h"
#include "eeconfig_wireless.h"

#define EECONFIG_BASE_ANALOG_MATRIX 37
#define EECONFIG_END_ANALOG_MATRIX (EECONFIG_BASE_ANALOG_MATRIX + EECONFIG_SIZE_ANALOG_MATRIX)

#define EECONFIG_BASE_WIRELESS_CONFIG EECONFIG_END_ANALOG_MATRIX
#define EECONFIG_END_WIRELESS_CONFIG (EECONFIG_BASE_WIRELESS_CONFIG + EECONFIG_SIZE_WIRELESS_CONFIG)

#define EECONFIG_KB_DATA_SIZE (EECONFIG_END_WIRELESS_CONFIG - EECONFIG_BASE_ANALOG_MATRIX)
