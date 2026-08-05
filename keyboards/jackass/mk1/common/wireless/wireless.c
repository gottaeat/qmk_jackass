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

#include "quantum.h"
#include "wireless.h"
#include "report_buffer.h"
#include "lpm.h"
#include "battery.h"
#include "indicator.h"
#include "transport.h"
#include "rtc_timer.h"
#include "keychron_wireless_common.h"
#include "wireless_config.h"
#include "eeprom.h"

static uint8_t host_index = 0;
static uint8_t led_state  = 0;

extern wt_func_t  wireless_transport;
static wt_state_t wireless_state = WT_RESET;
static bool       pincodeEntry   = false;

uint16_t connected_idle_time = CONNECTED_IDLE_TIME;

/* declarations */
static uint8_t wireless_keyboard_leds(void);
static void    wireless_send_keyboard(report_keyboard_t *report);
static void    wireless_send_nkro(report_nkro_t *report);
static void    wireless_send_mouse(report_mouse_t *report);
static void    wireless_send_extra(report_extra_t *report);

/* host struct */
host_driver_t wireless_driver = {
    wireless_keyboard_leds, wireless_send_keyboard, wireless_send_nkro, wireless_send_mouse, wireless_send_extra,
};

#define WT_EVENT_QUEUE_SIZE 16
static wireless_event_t wireless_event_queue[WT_EVENT_QUEUE_SIZE];
static uint8_t          wireless_event_queue_head;
static uint8_t          wireless_event_queue_tail;

static void wireless_event_queue_init(void) {
    // Initialise the event queue
    memset(&wireless_event_queue, 0, sizeof(wireless_event_queue));
    wireless_event_queue_head = 0;
    wireless_event_queue_tail = 0;
}

bool wireless_event_enqueue(wireless_event_t event) {
    uint8_t next = (wireless_event_queue_head + 1) % WT_EVENT_QUEUE_SIZE;
    if (next == wireless_event_queue_tail) {
        /* Override the first report */
        wireless_event_queue_tail = (wireless_event_queue_tail + 1) % WT_EVENT_QUEUE_SIZE;
    }
    wireless_event_queue[wireless_event_queue_head] = event;
    wireless_event_queue_head                       = next;
    return true;
}

static inline bool wireless_event_dequeue(wireless_event_t *event) {
    if (wireless_event_queue_head == wireless_event_queue_tail) {
        return false;
    }
    *event                    = wireless_event_queue[wireless_event_queue_tail];
    wireless_event_queue_tail = (wireless_event_queue_tail + 1) % WT_EVENT_QUEUE_SIZE;
    return true;
}

static void wireless_config_load(void) {
    eeprom_read_block(&connected_idle_time, (uint8_t *)EECONFIG_BASE_WIRELESS_CONFIG, sizeof(connected_idle_time));

    if (connected_idle_time == 0)
        connected_idle_time = CONNECTED_IDLE_TIME;
    else if (connected_idle_time < 30)
        connected_idle_time = 30;
}

/*
 * Bluetooth init.
 */
void wireless_init(void) {
    wireless_state = WT_INITIALIZED;

    wireless_event_queue_init();
    report_buffer_init();
    indicator_init();
    battery_init();
    lpm_init();
    rtc_timer_init();
    wireless_config_load();
}

/*
 * Enter pairing with specified host index and param
 */
void wireless_pairing_ex(uint8_t host_idx, void *param) {
    if (battery_is_critical_low()) return;
    if (wireless_transport.pairing_ex) wireless_transport.pairing_ex(host_idx, param);
    wireless_state = WT_PARING;

    host_index = host_idx;
}

/*
 * Initiate connection request to paired host
 */
void wireless_connect(void) {
    /*  Work around empty report after wakeup, which leads to reconneect/disconnected loop */
    if (battery_is_critical_low() || timer_read32() == 0) return;

    if (wireless_state == WT_RECONNECTING && !indicator_is_running()) {
        indicator_set(wireless_state, host_index);
    }
    wireless_transport.connect_ex(0, 0);
    wireless_state = WT_RECONNECTING;
}

/*
 * Initiate connection request to paired host with argument
 */
void wireless_connect_ex(uint8_t host_idx, uint16_t timeout) {
    if (battery_is_critical_low()) return;

    if (host_idx != 0) {
        /* Do nothing when trying to connect to current connected host*/
        if (host_index == host_idx && wireless_state == WT_CONNECTED) return;

        host_index = host_idx;
        led_state  = 0;
    }
    wireless_transport.connect_ex(host_idx, timeout);
    wireless_state = WT_RECONNECTING;
}

/* Initiate a disconnection */
void wireless_disconnect(void) {
    if (wireless_transport.disconnect) wireless_transport.disconnect();
}

/* Called when the BT device is reset. */
static void wireless_enter_reset(uint8_t reason) {
    wireless_state = WT_RESET;
    wireless_enter_reset_kb(reason);
}

/* Enters discoverable state. Upon entering this state we perform the following actions:
 *   - change state to WT_PARING
 *   - set pairing indication
 */
static void wireless_enter_discoverable(uint8_t host_idx) {
    host_index = host_idx;

    wireless_state = WT_PARING;
    indicator_set(wireless_state, host_idx);
}

/*
 * Enters reconnecting state. Upon entering this state we perform the following actions:
 *   - change state to RECONNECTING
 *   - set reconnect indication
 */
static void wireless_enter_reconnecting(uint8_t host_idx) {
    host_index = host_idx;

    wireless_state = WT_RECONNECTING;
    indicator_set(wireless_state, host_idx);
}

/* Enters connected state. Upon entering this state we perform the following actions:
 *   - change state to CONNECTED
 *   - set connected indication
 *   - enable NKRO if it is support
 */
static void wireless_enter_connected(uint8_t host_idx) {
    wireless_state = WT_CONNECTED;
    indicator_set(wireless_state, host_idx);
    host_index = host_idx;

    clear_keyboard();

    if (wireless_transport.update_bat_level) wireless_transport.update_bat_level(battery_get_percentage());
    lpm_timer_reset();
}

/* Enters disconnected state. Upon entering this state we perform the following actions:
 *   - change state to DISCONNECTED
 *   - set disconnected indication
 */
static void wireless_enter_disconnected(uint8_t host_idx, uint8_t reason) {
    uint8_t previous_state = wireless_state;
    led_state              = 0;
    if (get_transport() & TRANSPORT_WIRELESS) led_update_kb((led_t)led_state);

    wireless_state = WT_DISCONNECTED;

    if (previous_state == WT_CONNECTED) {
        lpm_timer_reset();
        indicator_set(WT_SUSPEND, host_idx);
    } else {
        indicator_set(wireless_state, host_idx);
    }

    report_buffer_init();
    wireless_enter_disconnected_kb(host_idx, reason);

    battery_timer_reset();
}

/* Enter pin code entry state. */
static void wireless_enter_bluetooth_pin_code_entry(void) {
    keymap_config.nkro = FALSE;
    pincodeEntry       = true;
}

/* Exit pin code entry state. */
static void wireless_exit_bluetooth_pin_code_entry(void) {
    eeconfig_read_keymap(&keymap_config);
    pincodeEntry = false;
}

bool is_wireless_pin_code_entry(void) {
    return pincodeEntry;
}

/* Enters disconnected state. Upon entering this state we perform the following actions:
 *   - change state to DISCONNECTED
 *   - set disconnected indication
 */
static void wireless_enter_sleep(void) {
    uint8_t prev_state = wireless_state;
    led_state          = 0;
    if (get_transport() & TRANSPORT_WIRELESS) led_update_kb((led_t)led_state);

    wireless_state = WT_SUSPEND;
    if (prev_state == WT_CONNECTED || prev_state == WT_PARING) {
        lpm_timer_reset();

        indicator_set(wireless_state, 0);
    }
}

static uint8_t wireless_keyboard_leds(void) {
    if (wireless_state == WT_CONNECTED) {
        return led_state;
    }

    return 0;
}

extern keymap_config_t keymap_config;

static void wireless_send_keyboard(report_keyboard_t *report) {
    if (battery_is_critical_low()) return;
    if (wireless_state == WT_PARING && !pincodeEntry) return;
    if (wireless_state == WT_CONNECTED || (wireless_state == WT_PARING && pincodeEntry)) {
        if (wireless_transport.send_keyboard) {
            bool empty = report_buffer_is_empty();

            report_buffer_t report_buffer;
            report_buffer.type = REPORT_TYPE_KB;
            memcpy(&report_buffer.keyboard, report, sizeof(report_keyboard_t));
            report_buffer_enqueue(&report_buffer);

            if (empty) report_buffer_task();
        }
    } else if (wireless_state != WT_RESET) {
        wireless_connect();
    }
}

static void wireless_send_nkro(report_nkro_t *report) {
    if (battery_is_critical_low()) return;

    if (wireless_state == WT_PARING && !pincodeEntry) return;

    if (wireless_state == WT_CONNECTED || (wireless_state == WT_PARING && pincodeEntry)) {
        if (wireless_transport.send_nkro) {
            bool empty = report_buffer_is_empty();

            report_buffer_t report_buffer;
            report_buffer.type = REPORT_TYPE_NKRO;
            memcpy(&report_buffer.nkro, report, sizeof(report_nkro_t));
            report_buffer_enqueue(&report_buffer);

            if (empty) report_buffer_task();
        }
    } else if (wireless_state != WT_RESET) {
        wireless_connect();
    }
}

static void wireless_send_mouse(report_mouse_t *report) {
    if (battery_is_critical_low()) return;

    if (wireless_state == WT_CONNECTED) {
        if (wireless_transport.send_mouse) wireless_transport.send_mouse((uint8_t *)report);
    } else if (wireless_state != WT_RESET) {
        wireless_connect();
    }
}

static void wireless_send_system(uint16_t data) {
    if (wireless_state == WT_CONNECTED) {
        if (wireless_transport.send_system) wireless_transport.send_system(data);
    } else if (wireless_state != WT_RESET) {
        wireless_connect();
    }
}

static void wireless_send_consumer(uint16_t data) {
    if (wireless_state == WT_CONNECTED) {
        if (report_buffer_is_empty() && report_buffer_next_inverval()) {
            if (wireless_transport.send_consumer) wireless_transport.send_consumer(data);
            report_buffer_update_timer();
        } else {
            report_buffer_t report_buffer;
            report_buffer.type     = REPORT_TYPE_CONSUMER;
            report_buffer.consumer = data;
            report_buffer_enqueue(&report_buffer);
        }
    } else if (wireless_state != WT_RESET) {
        wireless_connect();
    }
}

static void wireless_send_extra(report_extra_t *report) {
    if (battery_is_critical_low()) return;

    if (report->report_id == REPORT_ID_SYSTEM) {
        wireless_send_system(report->usage);
    } else if (report->report_id == REPORT_ID_CONSUMER) {
        wireless_send_consumer(report->usage);
    }
}

void wireless_low_battery_shutdown(void) {
    report_buffer_init();
    clear_keyboard(); //
    wait_ms(50);      // wait a while for bt module to free buffer by sending report

    // Release all keys by sending empty reports
    if (keymap_config.nkro) {
        report_nkro_t empty_nkro_report;
        memset(&empty_nkro_report, 0, sizeof(empty_nkro_report));
        wireless_transport.send_nkro(&empty_nkro_report.mods);
    } else {
        report_keyboard_t empty_report;
        memset(&empty_report, 0, sizeof(empty_report));
        wireless_transport.send_keyboard(&empty_report.mods);
    }
    wait_ms(10);
    wireless_transport.send_consumer(0);
    wait_ms(10);
    report_mouse_t empty_mouse_report;
    memset(&empty_mouse_report, 0, sizeof(empty_mouse_report));
    wireless_transport.send_mouse((uint8_t *)&empty_mouse_report);
    wait_ms(300); // Wait for bt module to send all buffered report

    wireless_disconnect();
}

void wireless_event_task(void) {
    wireless_event_t event;
    while (wireless_event_dequeue(&event)) {
        switch (event.evt_type) {
            case EVT_RESET:
                wireless_enter_reset(event.params.reason);
                break;
            case EVT_CONNECTED:
                wireless_enter_connected(event.params.hostIndex);
                break;
            case EVT_DISCOVERABLE:
                wireless_enter_discoverable(event.params.hostIndex);
                break;
            case EVT_RECONNECTING:
                wireless_enter_reconnecting(event.params.hostIndex);
                break;
            case EVT_DISCONNECTED:
                wireless_enter_disconnected(event.params.hostIndex, event.data);
                break;
            case EVT_BT_PINCODE_ENTRY:
                wireless_enter_bluetooth_pin_code_entry();
                break;
            case EVT_EXIT_BT_PINCODE_ENTRY:
                wireless_exit_bluetooth_pin_code_entry();
                break;
            case EVT_SLEEP:
                wireless_enter_sleep();
                break;
            case EVT_HID_INDICATOR:
                led_state = event.params.led;
                break;
            case EVT_CONECTION_INTERVAL:
                report_buffer_set_inverval(event.params.interval);
                break;
            default:
                break;
        }
    }
}

void wireless_task(void) {
    wireless_transport.task();
    wireless_event_task();
    report_buffer_task();
    indicator_task();
    keychron_wireless_common_task();
    battery_task();
    lpm_task();
}

wt_state_t wireless_get_state(void) {
    return wireless_state;
};

bool process_record_wireless(uint16_t keycode, keyrecord_t *record) {
    if (get_transport() & TRANSPORT_WIRELESS) {
        lpm_timer_reset();
    }

    if (!process_record_keychron_wireless(keycode, record)) return false;

    return true;
}
