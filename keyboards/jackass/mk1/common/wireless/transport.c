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
#include "indicator.h"
#include "lpm.h"
#include "mousekey.h"
#include <usb_main.h>
#include "transport.h"
#include "lkbt51.h"
#include "snled27351-spi.h"

extern host_driver_t   chibios_driver;
extern host_driver_t   wireless_driver;
extern keymap_config_t keymap_config;
extern wt_func_t       wireless_transport;

static transport_t transport = TRANSPORT_NONE;

static void transport_changed(transport_t new_transport);

static void bt_transport_enable(bool enable) {
    if (enable) {
        host_set_driver(&wireless_driver);

        /* Disconnect and reconnect to sync the wireless state
         * TODO: query wireless state to sync
         */
        wireless_disconnect();

        uint32_t t = timer_read32();
        while (timer_elapsed32(t) < 100) {
            wireless_transport.task();
        }
        wireless_connect_ex(30, 0);
    } else {
        indicator_stop();

        if (wireless_get_state() == WT_CONNECTED && transport == TRANSPORT_BLUETOOTH) {
            report_keyboard_t empty_report = {0};
            wireless_driver.send_keyboard(&empty_report);
        }
    }
}

static void p24g_transport_enable(bool enable) {
    if (enable) {
        host_set_driver(&wireless_driver);

        /* Disconnect and reconnect to sync the wireless state
         * TODO: query bluetooth state to sync
         */
        wireless_disconnect();

        uint32_t t = timer_read32();
        while (timer_elapsed32(t) < 100) {
            wireless_transport.task();
        }
        wireless_connect_ex(P24G_INDEX, 0);
    } else {
        indicator_stop();

        if (wireless_get_state() == WT_CONNECTED && transport == TRANSPORT_P2P4) {
            report_keyboard_t empty_report = {0};
            wireless_driver.send_keyboard(&empty_report);
        }
    }
}

static void usb_transport_enable(bool enable) {
    if (enable) {
        if (host_get_driver() != &chibios_driver) {
            host_set_driver(&chibios_driver);
        }
    } else {
        if (USB_DRIVER.state == USB_ACTIVE) {
            report_keyboard_t empty_report = {0};
            chibios_driver.send_keyboard(&empty_report);
        }
    }
}

void set_transport(transport_t new_transport) {
    if (transport != new_transport) {
        if (transport == TRANSPORT_USB || ((transport != TRANSPORT_USB) && wireless_get_state() == WT_CONNECTED)) {
            clear_keyboard();
        }

        transport = new_transport;

        switch (transport) {
            case TRANSPORT_USB:
                usb_transport_enable(true);
                bt_transport_enable(false);
                wait_ms(5);
                p24g_transport_enable(false);
                wireless_disconnect();
                lpm_timer_stop();
                transport_changed(transport);
                break;

            case TRANSPORT_BLUETOOTH:
                p24g_transport_enable(false);
                wait_ms(1);
                bt_transport_enable(true);
                usb_transport_enable(false);
                lpm_timer_reset();
                transport_changed(transport);
                break;

            case TRANSPORT_P2P4:
                bt_transport_enable(false);
                wait_ms(1);
                p24g_transport_enable(true);
                usb_transport_enable(false);
                lpm_timer_reset();
                transport_changed(transport);
                break;

            default:
                break;
        }
    }
}

transport_t get_transport(void) {
    return transport;
}

/* Changing transport may cause bronw-out reset of led driver
 * without MCU reset, which lead backlight to not work,
 * reinit the led driver workgound this issue */
static void reinit_led_driver(transport_t new_transport) {
    bool    backlight_enabled = rgb_matrix_is_enabled();
    uint8_t brightness        = rgb_matrix_get_val();

    snled27351_shutdown();

    /* Wait circuit to discharge for a while */
    wait_ms(100);

    rgb_matrix_init();
    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv_noeeprom(0, 0, brightness);

    if (backlight_enabled && (new_transport != TRANSPORT_USB || usb_power_connected())) {
        rgb_matrix_set_color_all(brightness, brightness, brightness);
    } else {
        rgb_matrix_set_color_all(0, 0, 0);
    }
    if (!backlight_enabled) {
        rgb_matrix_disable_noeeprom();
    }
    rgb_matrix_update_pwm_buffers();
}

static void transport_changed(transport_t new_transport) {
    indicator_init();

    reinit_led_driver(new_transport);

    led_update_kb(host_keyboard_led_state());
}

void usb_remote_wakeup(void) {
    usb_event_queue_task();

    if (USB_DRIVER.state == USB_SUSPENDED) {
        while (USB_DRIVER.state == USB_SUSPENDED) {
            wireless_pre_task();
            if (get_transport() != TRANSPORT_USB) {
                suspend_wakeup_init_quantum();
                return;
            }
            /* Do this in the suspended state */
            suspend_power_down();
            /* Remote wakeup */
            if (suspend_wakeup_condition()) {
                usbWakeupHost(&USB_DRIVER);
                wait_ms(300);
                // Wiggle to wakeup
                mousekey_on(MS_LEFT);
                mousekey_send();
                wait_ms(10);
                mousekey_on(MS_RGHT);
                mousekey_send();
                wait_ms(10);
                mousekey_off((MS_RGHT));
                mousekey_send();
            }
        }
        /* Woken up */
        send_keyboard_report();
        mousekey_send();
    }
}
