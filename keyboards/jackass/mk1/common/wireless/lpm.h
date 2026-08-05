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
#include "config.h"
#include "lpm_stm32f401.h"

#define DEFAULT_PROCESS_TIME 3000
#define CONNECTED_PROCESS_TIME (30 * 60 * 1000)

void lpm_init(void);
void lpm_timer_reset(void);
void lpm_timer_stop(void);
void matrix_enter_low_power(void);
void matrix_exit_low_power(void);
void lpm_enter_low_power(void);
void lpm_post_enter_low_power(void);
void lpm_standby(pm_t mode);
void lpm_wakeup_init(void);
void lpm_pre_wakeup(void);
void lpm_wakeup(void);
bool usb_power_connected(void);
bool lpm_is_kb_idle(void);
bool lpm_set(pm_t mode);
void lpm_task(void);
