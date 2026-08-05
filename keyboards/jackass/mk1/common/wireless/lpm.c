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

/******************************************************************************
 *
 *  Filename:      lpm.c
 *
 *  Description:   Contains low power mode implementation
 *
 ******************************************************************************/

#include "quantum.h"
#include <usb_main.h>
#include "debounce.h"
#include "wireless.h"
#include "indicator.h"
#include "lpm.h"
#include "transport.h"
#include "battery.h"
#include "report_buffer.h"
#include "keychron_common.h"
#include "snled27351-spi.h"

void debounce_free(void);

extern matrix_row_t matrix[MATRIX_ROWS];
extern wt_func_t    wireless_transport;

static uint32_t     lpm_timer_buffer;
static bool         lpm_time_up               = false;
static uint32_t     lpm_reset_time            = DEFAULT_PROCESS_TIME;
static matrix_row_t empty_matrix[MATRIX_ROWS] = {0};
static pin_t        dip_switch_pins[]         = DIP_SWITCH_PINS;

void lpm_init(void) {
    gpio_set_pin_input_high(USB_POWER_SENSE_PIN);
    lpm_timer_reset();
}

inline void lpm_timer_reset(void) {
    lpm_time_up      = false;
    lpm_timer_buffer = timer_read32();

    if (wireless_get_state() == WT_CONNECTED)
        lpm_reset_time = CONNECTED_PROCESS_TIME;
    else
        lpm_reset_time = DEFAULT_PROCESS_TIME;
}

void lpm_timer_stop(void) {
    lpm_time_up      = false;
    lpm_timer_buffer = 0;
}

static inline bool lpm_any_matrix_action(void) {
    return memcmp(matrix, empty_matrix, sizeof(empty_matrix));
}

static void lpm_turn_off_backlight_and_led(void) {
    rgb_matrix_set_color_all(0, 0, 0);
    rgb_matrix_driver.flush();
    snled27351_shutdown();
}

/* Implement of entering low power mode and wakeup varies per mcu or platform */

void lpm_enter_low_power(void) {
    if (get_transport() == TRANSPORT_USB && !usb_power_connected()) {
        lpm_turn_off_backlight_and_led();
    }

    /* Usb unit is actived and running, stop and disconnect first */
    usbStop(&USBD1);
    usbDisconnectBus(&USBD1);

    spiStop(&SPI_DRIVER);
    palSetLineMode(SPI_SCK_PIN, PAL_MODE_INPUT_PULLDOWN);
    palSetLineMode(SPI_MOSI_PIN, PAL_MODE_INPUT_PULLDOWN);
    palSetLineMode(SPI_MISO_PIN, PAL_MODE_INPUT_PULLDOWN);

    palEnableLineEvent(WIRELESS_TO_MCU_INT_PIN, PAL_EVENT_MODE_FALLING_EDGE);
    palEnableLineEvent(USB_POWER_SENSE_PIN, PAL_EVENT_MODE_BOTH_EDGES);
    palEnableLineEvent(P24G_MODE_SELECT_PIN, PAL_EVENT_MODE_BOTH_EDGES);
    palEnableLineEvent(BT_MODE_SELECT_PIN, PAL_EVENT_MODE_BOTH_EDGES);
    matrix_enter_low_power();

    for (uint8_t i = 0; i < ARRAY_SIZE(dip_switch_pins); i++) {
        gpio_set_pin_input_low(dip_switch_pins[i]);
    }
}

void lpm_early_wakeup(void) {
    gpio_write_pin_low(MCU_TO_WIRELESS_INT_PIN);
}

void lpm_wakeup(void) {
    halInit();
    snled27351_exit_shutdown();
    if (wireless_transport.init) wireless_transport.init(true);

    /* Init dip switch as early as possible, make sure first read later is correct. */
    dip_switch_init();
    battery_init();

    matrix_exit_low_power();
    palDisableLineEvent(WIRELESS_TO_MCU_INT_PIN);
    palDisableLineEvent(P24G_MODE_SELECT_PIN);
    palDisableLineEvent(BT_MODE_SELECT_PIN);
    palDisableLineEvent(USB_POWER_SENSE_PIN);

    if (usb_power_connected()) {
        usb_event_queue_init();
        init_usb_driver(&USB_DRIVER);
    }

    /* Call debounce_free() to avoiding memory leak of debounce_counters as debounce_init()
    invoked in matrix_init() alloc new memory to debounce_counters */
    debounce_free();
    matrix_init();

    dip_switch_read(true);
}

bool usb_power_connected(void) {
    return gpio_read_pin(USB_POWER_SENSE_PIN) == USB_POWER_CONNECTED_LEVEL;
}

bool allow_low_power_mode(pm_t mode) {
    /* Don't enter low power mode if attached to the host */
    if (mode > PM_SLEEP && usb_power_connected()) return false;

    if (!lpm_set(mode)) return false;

    return true;
}

void lpm_task(void) {
    if (!lpm_time_up && timer_elapsed32(lpm_timer_buffer) > lpm_reset_time) {
        lpm_time_up      = true;
        lpm_timer_buffer = 0;
    }

    if (usb_power_connected()) {
        if (USBD1.state == USB_STOP) {
            /* TODO: Do we need this? */
            usb_event_queue_init();
            init_usb_driver(&USB_DRIVER);
        }
        /* No need to enter low power mode when USB power is connected */
        return;
    }

    switch (get_transport()) {
        case TRANSPORT_BLUETOOTH:
        case TRANSPORT_P2P4:
            break;
        default:
            return;
    }

    if (!(lpm_time_up && !indicator_is_running() && lpm_is_kb_idle())) return;

    if (lpm_any_matrix_action() || !allow_low_power_mode(LOW_POWER_MODE)) return;

    lpm_enter_low_power();
    lpm_post_enter_low_power();

    lpm_standby(LOW_POWER_MODE);
    lpm_early_wakeup();
    lpm_wakeup_init();

    lpm_pre_wakeup();
    lpm_wakeup();
    lpm_timer_reset();
    report_buffer_init();
    lpm_set(PM_RUN);
}
