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

#include "quantum.h"
#include "wireless.h"
#include "battery.h"
#include "transport.h"
#include "lkbt51.h"
#include "lpm.h"
#include "indicator.h"
#include "rtc_timer.h"
#include "config.h"
#include "snled27351-spi.h"

#define CRITICAL_LOW_COUNT 20

#define LKBT51_RVD_R1 560
#define LKBT51_RVD_R2 499
#define VOLTAGE_TRIM_RGB_MATRIX 60

static uint32_t bat_monitor_timer_buffer = 0;
static uint16_t voltage                  = FULL_VOLTAGE_VALUE;
static uint8_t  critical_low             = 0;
static uint8_t  power_on_sample          = 0;

void battery_init(void) {
    palSetLineMode(BAT_CHARGING_PIN, PAL_MODE_INPUT_PULLUP);
}

static void battery_measure(void) {
    lkbt51_read_state_reg(0x05, 0x02);
}

/* Calculate the voltage */
void battery_calculate_voltage(uint16_t value) {
    uint16_t measured_voltage = ((uint32_t)value) * (LKBT51_RVD_R1 + LKBT51_RVD_R2) / LKBT51_RVD_R2;

    if (rgb_matrix_is_enabled()) {
        /* Account for LED load while measuring battery voltage. */
        uint32_t compensation = VOLTAGE_TRIM_RGB_MATRIX * snled27351_get_total_duty_ratio();
        measured_voltage += compensation;
    }

    voltage = measured_voltage;
}

uint8_t battery_get_percentage(void) {
    if (voltage > FULL_VOLTAGE_VALUE) return 100;

    if (voltage > EMPTY_VOLTAGE_VALUE) {
        return ((uint32_t)voltage - EMPTY_VOLTAGE_VALUE) * 80 / (FULL_VOLTAGE_VALUE - EMPTY_VOLTAGE_VALUE) + 20;
    }

    if (voltage > SHUTDOWN_VOLTAGE_VALUE) {
        return ((uint32_t)voltage - SHUTDOWN_VOLTAGE_VALUE) * 20 / (EMPTY_VOLTAGE_VALUE - SHUTDOWN_VOLTAGE_VALUE);
    } else
        return 0;
}

bool battery_is_critical_low(void) {
    return critical_low > CRITICAL_LOW_COUNT;
}

static void battery_check_critical_low(void) {
    if (voltage < SHUTDOWN_VOLTAGE_VALUE) {
        if (critical_low <= CRITICAL_LOW_COUNT) {
            if (++critical_low > CRITICAL_LOW_COUNT) wireless_low_battery_shutdown();
        }
    } else if (critical_low <= CRITICAL_LOW_COUNT) {
        critical_low = 0;
    }
}

static bool battery_power_on_sample(void) {
    return power_on_sample < VOLTAGE_POWER_ON_MEASURE_COUNT;
}

void battery_timer_reset(void) {
    bat_monitor_timer_buffer = rtc_timer_read_ms();
}

void battery_task(void) {
    uint32_t t = rtc_timer_elapsed_ms(bat_monitor_timer_buffer);
    if ((get_transport() & TRANSPORT_WIRELESS) && (wireless_get_state() == WT_CONNECTED || battery_power_on_sample())) {
        if (usb_power_connected() && t > VOLTAGE_MEASURE_INTERVAL) {
            if (gpio_read_pin(BAT_CHARGING_PIN) == BAT_CHARGING_LEVEL)
                lkbt51_update_bat_state(BAT_CHARGING);
            else
                lkbt51_update_bat_state(BAT_FULL_CHARGED);
        }

        if ((battery_power_on_sample() && !rgb_matrix_is_enabled() && t > BACKLIGHT_OFF_VOLTAGE_MEASURE_INTERVAL) || t > VOLTAGE_MEASURE_INTERVAL) {
            battery_check_critical_low();

            bat_monitor_timer_buffer = rtc_timer_read_ms();
            if (bat_monitor_timer_buffer > RTC_MAX_TIME) {
                bat_monitor_timer_buffer = 0;
                rtc_timer_clear();
            }

            battery_measure();
            if (power_on_sample < VOLTAGE_POWER_ON_MEASURE_COUNT) power_on_sample++;
        }
    }

    if (critical_low && usb_power_connected()) {
        critical_low = false;
    }
}
