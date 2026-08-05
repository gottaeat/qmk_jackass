/* Copyright 2024 ~ 2025 @ Keychron (https://www.keychron.com)
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

#include "eeconfig_kb.h"

/* External EEPROM Configuration*/
#define I2C_DRIVER I2CD3
#define I2C1_SCL_PIN A8
#define I2C1_SDA_PIN C9
#define EXTERNAL_EEPROM_WP_PIN B10

/* Analog Matrix Configuration */
#define ANALOG_MATRIX_POWER_PIN C13
#define ANALOG_MATRIX_POWER_ENABLE_LEVEL 1
#define ANALOG_MATRIX_WAKEUP_PIN C5

/* SPI Configuration */
#define SPI_DRIVER SPID1
#define SPI_SCK_PIN A5
#define SPI_MISO_PIN A6
#define SPI_MOSI_PIN A7

/* SNLED27351 Driver Configuration */
#define SNLED27351_SELECT_PINS {B8, B9}
#define SNLED27351_SDB_PIN B7
#define SNLED27351_PHASE_CHANNEL SNLED27351_SCAN_PHASE_9_CHANNEL
#define SNLED27351_SPI_DIVISOR 16

/* Wireless Configuration */
#define P24G_MODE_SELECT_PIN A9
#define BT_MODE_SELECT_PIN A10
#define LKBT51_RESET_PIN C4
#define WIRELESS_TO_MCU_INT_PIN B1
#define MCU_TO_WIRELESS_INT_PIN A4
#define USB_POWER_SENSE_PIN B0
#define USB_POWER_CONNECTED_LEVEL 0
#define BAT_CHARGING_PIN B13
#define BAT_CHARGING_LEVEL 0
#define BAT_LEVEL_LED_LIST {17, 18, 19, 20, 21, 22, 23, 24, 25, 26}

/* LED Current Configuration */
#define SNLED27351_CURRENT_TUNE {0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C}

/* RGB Matrix Configuration */
#define RGB_MATRIX_LED_COUNT 84
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#define RGB_MATRIX_DEFAULT_SAT 0
#define RGB_MATRIX_DISABLE_SHARED_KEYCODES

/* Indications */
#define CAPS_LOCK_INDEX 46
