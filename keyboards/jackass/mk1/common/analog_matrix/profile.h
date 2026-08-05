/* Copyright 2024 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "action.h"

void    profile_init(bool reset);
uint8_t profile_get_actuation_point(void);
bool    process_record_profile(uint16_t keycode, keyrecord_t *record);

void profile_indication_timer_check(void);
void profile_indication(void);
bool profile_indication_active(void);
