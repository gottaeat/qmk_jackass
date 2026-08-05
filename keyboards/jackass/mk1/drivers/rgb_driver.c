/* Copyright 2021 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "rgb_matrix_drivers.h"
#include "snled27351-spi.h"

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = snled27351_init_drivers,
    .flush         = snled27351_flush,
    .set_color     = snled27351_set_color,
    .set_color_all = snled27351_set_color_all,
};
