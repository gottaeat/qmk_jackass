/* Copyright 2023 ~ 2025 @ Keychron (https://www.keychron.com)
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

#include "stdint.h"
#include "hal.h"
#include "config.h"

// Error checking
#if HAL_USE_SPI == FALSE
#    error "Please enable SPI to use LKBT51"
#endif

#define WT_DRIVER SPID1

#define PACKET_MAX_LEN 64
#define P24G_INDEX 24

typedef struct {
    uint8_t  event_mode; /* Must be 0x02 */
    uint16_t connected_idle_timeout;
    uint16_t pairing_timeout;   /* Range: 30 ~ 3600 second, 0 for default */
    uint8_t  pairing_mode;      /* 0: default, 1: Just Works, 2: Passkey Entry */
    uint16_t reconnect_timeout; /* 0: default, 0xFF: Unlimited time, 2 ~ 254 seconds */
    uint8_t  report_rate;       /* 90 or 133 */
    uint8_t  rsvd1;
    uint8_t  rsvd2;
    uint8_t  vendor_id_source; /* 0: From Bluetooth SIG, 1: From USB-IF */
    uint16_t verndor_id;       /* No effect, the vendor ID is 0x3434 */
    uint16_t product_id;
    /* Below parametes is only available for BLE module  */
    uint16_t le_connection_interval_min;
    uint16_t le_connection_interval_max;
    uint16_t le_connection_interval_timeout;
} __attribute__((packed)) module_param_t;

void lkbt51_init(bool wakeup_from_low_power_mode);

void lkbt51_send_keyboard(uint8_t* report);
void lkbt51_send_nkro(uint8_t* report);
void lkbt51_send_consumer(uint16_t report);
void lkbt51_send_system(uint16_t report);
void lkbt51_send_mouse(uint8_t* report);

void lkbt51_become_discoverable(uint8_t host_idx, void* param);
void lkbt51_connect(uint8_t hostIndex, uint16_t timeout);
void lkbt51_disconnect(void);
void lkbt51_read_state_reg(uint8_t reg, uint8_t len);

void lkbt51_update_bat_lvl(uint8_t bat_lvl);
void lkbt51_update_bat_state(uint8_t bat_state);

void lkbt51_set_param(module_param_t* param);
void lkbt51_set_local_name(const char* name);

void lkbt51_task(void);
