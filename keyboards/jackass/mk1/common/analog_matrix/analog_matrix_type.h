/* Copyright 2024 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdint.h>

enum {
    AKS_REGULAR_RELEASED,
    AKS_REGULAR_PRESSED,
};

#pragma pack(push)
#pragma pack(1)

typedef struct __attribute__((__packed__)) {
    uint8_t actn_pt;
    uint8_t deactn_pt;
} activity_point_t;

typedef struct __attribute__((__packed__)) {
    uint8_t          state;
    uint8_t          travel;
    uint8_t          last_travel;
    uint16_t         last_val;
    activity_point_t regular;
} analog_key_t;

typedef struct __attribute__((__packed__)) {
    uint16_t zero_travel : 12;
    uint16_t full_travel : 12;
} calibrated_value_t;

typedef struct __attribute__((__packed__)) {
    uint8_t            pressed : 1;
    uint8_t            state : 3;
    uint8_t            confidence;
    calibrated_value_t value;
    uint32_t           full_travel_time;
    uint8_t            cycle;
} calibration_t;

#pragma pack(pop)
