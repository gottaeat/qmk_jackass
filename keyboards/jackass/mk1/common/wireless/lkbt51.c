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

#include "quantum.h"
#include "lkbt51.h"
#include "wireless.h"
#include "wireless_event_type.h"
#include "battery.h"
#include "report_buffer.h"

#define SPI_CLK_PAL_MODE 5
#define SPI_MISO_PAL_MODE 5
#define SPI_MOSI_PAL_MODE 5
#define LKBT51_COMM_TIMEOUT_MS 2000

// clang-format off
enum {
    /* HID Report  */
    LKBT51_CMD_SEND_KB       = 0x11,
    LKBT51_CMD_SEND_KB_NKRO  = 0x12,
    LKBT51_CMD_SEND_CONSUMER = 0x13,
    LKBT51_CMD_SEND_SYSTEM   = 0x14,
    LKBT51_CMD_SEND_MOUSE    = 0x16,
    /* Bluetooth connections */
    LKBT51_CMD_PAIRING        = 0x21,
    LKBT51_CMD_CONNECT        = 0x22,
    LKBT51_CMD_DISCONNECT     = 0x23,
    LKBT51_CMD_READ_STATE_REG = 0x25,
    /* Battery */
    LKBT51_CMD_UPDATE_BAT_LVL = 0x32,
    LKBT51_CMD_UPDATE_BAT_STATE = 0x33,
    /* Set/get parameters */
    LKBT51_CMD_SET_CONFIG      = 0x41,
    LKBT51_CMD_SET_NAME        = 0x45,
    /* Event */
    LKBT51_CONNECTION_EVT_ACK = 0xA4,
};

enum {
    LKBT51_EVT_ACK           = 0xA1,
    LKBT51_EVT_RESET         = 0xB0,
    LKBT51_EVT_HID_EVENT     = 0xB4,
};

enum {
    LKBT51_CONNECTED = 0x20,
    LKBT51_DISCOVERABLE = 0x21,
    LKBT51_RECONNECTING = 0x22,
    LKBT51_DISCONNECTED = 0x23,
    LKBT51_PINCODE_ENTRY = 0x24,
    LKBT51_EXIT_PINCODE_ENTRY = 0x25,
    LKBT51_SLEEP = 0x26
};

enum {
    ACK_SUCCESS = 0x00,
    ACK_CHECKSUM_ERROR,
    ACK_FIFO_HALF_WARNING,
    ACK_FIFO_FULL_ERROR,
};

enum{
    LK_EVT_MSK_CONNECTION = 0x01 << 0,
    LK_EVT_MSK_LED = 0x01 << 1,
    LK_EVT_MSK_BATT = 0x01 << 2,
    LK_EVT_MSK_RESET = 0x01 << 3,
    LK_EVT_MSK_RPT_INTERVAL = 0x01 << 4,
};

// clang-format on

static uint8_t  payload[PACKET_MAX_LEN];
static uint8_t  expect_len          = 22;
static uint16_t connection_interval = 1;
static uint32_t wake_time;

static volatile uint32_t lkbt51_last_comm_time = 0;

// clang-format off
wt_func_t wireless_transport = {
    lkbt51_init,
    lkbt51_connect,
    lkbt51_become_discoverable,
    lkbt51_disconnect,
    lkbt51_send_keyboard,
    lkbt51_send_nkro,
    lkbt51_send_consumer,
    lkbt51_send_system,
    lkbt51_send_mouse,
    lkbt51_update_bat_lvl,
    lkbt51_task
};
// clang-format on

/* Init SPI */
static const SPIConfig spicfg = {
    .circular = false,
    .slave    = false,
    .data_cb  = NULL,
    .error_cb = NULL,
    .ssport   = PAL_PORT(MCU_TO_WIRELESS_INT_PIN),
    .sspad    = PAL_PAD(MCU_TO_WIRELESS_INT_PIN),
    .cr1      = SPI_CR1_MSTR | SPI_CR1_BR_1 | SPI_CR1_BR_0,
    .cr2      = 0U,
};

void lkbt51_init(bool wakeup_from_low_power_mode) {
    if (!wakeup_from_low_power_mode) {
        gpio_set_pin_output_push_pull(LKBT51_RESET_PIN);
        gpio_write_pin_low(LKBT51_RESET_PIN);
        wait_ms(1);
        gpio_write_pin_high(LKBT51_RESET_PIN);
    }

    palSetLineMode(SPI_SCK_PIN, PAL_MODE_ALTERNATE(SPI_CLK_PAL_MODE));
    palSetLineMode(SPI_MISO_PIN, PAL_MODE_ALTERNATE(SPI_MISO_PAL_MODE));
    palSetLineMode(SPI_MOSI_PIN, PAL_MODE_ALTERNATE(SPI_MOSI_PAL_MODE));

    if (WT_DRIVER.state == SPI_UNINIT) {
        if (wakeup_from_low_power_mode) {
            spiInit();
            return;
        }

        spiInit();
    }

    gpio_set_pin_output_push_pull(MCU_TO_WIRELESS_INT_PIN);
    gpio_write_pin_high(MCU_TO_WIRELESS_INT_PIN);

    gpio_set_pin_input_high(WIRELESS_TO_MCU_INT_PIN);
}

static inline void lkbt51_wake(void) {
    if (timer_elapsed32(wake_time) > 3000) {
        wake_time = timer_read32();

        palWriteLine(MCU_TO_WIRELESS_INT_PIN, 0);
        wait_ms(10);
        palWriteLine(MCU_TO_WIRELESS_INT_PIN, 1);
        wait_ms(300);
    }
}

static void lkbt51_send_cmd(uint8_t* payload, uint8_t len, bool ack_enable) {
    static uint8_t sn = 0;
    uint8_t        i;
    uint8_t        pkt[PACKET_MAX_LEN] = {0};
    memset(pkt, 0, PACKET_MAX_LEN);

    ++sn;
    if (sn == 0) ++sn;

    uint16_t checksum = 0;
    for (i = 0; i < len; i++)
        checksum += payload[i];

    i        = 0;
    pkt[i++] = 0x84;
    pkt[i++] = 0x7e;
    pkt[i++] = 0x00;
    pkt[i++] = 0x00;
    pkt[i++] = 0xAA;
    pkt[i++] = ack_enable ? 0x56 : 0x55;
    pkt[i++] = len + 2;
    pkt[i++] = ~(len + 2) & 0xFF;
    pkt[i++] = sn;

    memcpy(pkt + i, payload, len);
    i += len;
    pkt[i++] = checksum & 0xFF;
    pkt[i++] = (checksum >> 8) & 0xFF;

    expect_len = 64;

    spiStart(&WT_DRIVER, &spicfg);
    spiSelect(&WT_DRIVER);
    spiSend(&WT_DRIVER, i, pkt);
    spiUnselectI(&WT_DRIVER);
    spiStop(&WT_DRIVER);
}

static void lkbt51_read(uint8_t* payload, uint8_t len) {
    uint8_t i;
    uint8_t pkt[PACKET_MAX_LEN] = {0};
    memset(pkt, 0, PACKET_MAX_LEN);

    i        = 0;
    pkt[i++] = 0x84;
    pkt[i++] = 0x7f;
    pkt[i++] = 0x00;
    pkt[i++] = 0x80;

    i += len;

    spiStart(&WT_DRIVER, &spicfg);
    spiSelect(&WT_DRIVER);
    spiExchange(&WT_DRIVER, i, pkt, payload);
    spiUnselect(&WT_DRIVER);
    spiStop(&WT_DRIVER);
}

void lkbt51_send_keyboard(uint8_t* report) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SEND_KB;
    memcpy(payload + i, report, 8);
    i += 8;

    lkbt51_send_cmd(payload, i, true);
    if (!lkbt51_last_comm_time) {
        lkbt51_last_comm_time = timer_read32();
    }
}

void lkbt51_send_nkro(uint8_t* report) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SEND_KB_NKRO;
    memcpy(payload + i, report, 20); // NKRO report lenght is limited to 20 bytes
    i += 20;

    lkbt51_send_cmd(payload, i, true);
    if (!lkbt51_last_comm_time) {
        lkbt51_last_comm_time = timer_read32();
    }
}

void lkbt51_send_consumer(uint16_t report) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SEND_CONSUMER;
    payload[i++] = report & 0xFF;
    payload[i++] = ((report) >> 8) & 0xFF;
    i += 4; // QMK doesn't send multiple consumer reports, just skip 2nd and 3rd consumer reports

    lkbt51_send_cmd(payload, i, true);
    if (!lkbt51_last_comm_time) {
        lkbt51_last_comm_time = timer_read32();
    }
}

void lkbt51_send_system(uint16_t report) {
    uint8_t hid_usage = report & 0xFF;

    if ((hid_usage < 0x81 || hid_usage > 0x83) && hid_usage != 0x9B && hid_usage != 0x00) return;

    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SEND_SYSTEM;
    if (hid_usage == 0x00) {
        payload[i++] = 0x00;
    } else if (hid_usage >= 0x81 && hid_usage <= 0x83) {
        payload[i++] = 0x01 << (hid_usage - 0x81);
    } else if (hid_usage == 0x9B) {
        payload[i++] = 0x08;
    }

    lkbt51_send_cmd(payload, i, true);
    if (!lkbt51_last_comm_time) {
        lkbt51_last_comm_time = timer_read32();
    }
}

void lkbt51_send_mouse(uint8_t* report) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SEND_MOUSE;            // Cmd type
    payload[i++] = report[1];                        // Button
    payload[i++] = report[2];                        // X
    payload[i++] = (report[2] & 0x80) ? 0xff : 0x00; // ckbt51 use 16bit report, set high byte
    payload[i++] = report[3];                        // Y
    payload[i++] = (report[3] & 0x80) ? 0xff : 0x00; // ckbt51 use 16bit report, set high byte
    payload[i++] = report[4];                        // V wheel
    payload[i++] = report[5];                        // H wheel

    lkbt51_send_cmd(payload, i, false);
}

/* Send ack to connection event, wireless module will retry 2 times if no ack received */
static void lkbt51_send_conn_evt_ack(void) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CONNECTION_EVT_ACK;

    lkbt51_send_cmd(payload, i, false);
}

void lkbt51_become_discoverable(uint8_t host_idx, void* param) {
    uint8_t i = 0;
    (void)param;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_PAIRING; // Cmd type
    payload[i++] = host_idx;           // Host Index
    payload[i++] = 0;                  // Default timeout
    payload[i++] = 0;
    payload[i++] = 3; // LESC or SSP
    payload[i++] = 1; // Bluetooth Classic
    payload[i++] = 0; // Default transmit power

    lkbt51_wake();
    lkbt51_send_cmd(payload, i, true);
}

/* Timeout : 2 ~ 255 seconds */
void lkbt51_connect(uint8_t hostIndex, uint16_t timeout) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_CONNECT;
    payload[i++] = hostIndex;      // Host index
    payload[i++] = timeout & 0xFF; // Timeout
    payload[i++] = (timeout >> 8) & 0xFF;

    lkbt51_wake();
    lkbt51_send_cmd(payload, i, true);
    if (!lkbt51_last_comm_time) {
        lkbt51_last_comm_time = timer_read32();
    }
}

void lkbt51_disconnect(void) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_DISCONNECT;
    payload[i++] = 0; // Sleep mode

    if (WT_DRIVER.state != SPI_READY) spiStart(&WT_DRIVER, &spicfg);
    spiSelect(&WT_DRIVER);
    wait_ms(30);
    // spiUnselect(&WT_DRIVER);
    wait_ms(70);

    lkbt51_send_cmd(payload, i, true);
}

void lkbt51_read_state_reg(uint8_t reg, uint8_t len) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_READ_STATE_REG;
    payload[i++] = reg;
    payload[i++] = len;

    // TODO
    lkbt51_send_cmd(payload, i, false);
}

void lkbt51_update_bat_lvl(uint8_t bat_lvl) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_UPDATE_BAT_LVL;
    payload[i++] = bat_lvl;
    lkbt51_send_cmd(payload, i, false);
}

void lkbt51_update_bat_state(uint8_t bat_state) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_UPDATE_BAT_STATE;
    payload[i++] = bat_state;
    lkbt51_send_cmd(payload, i, false);
}

void lkbt51_set_param(module_param_t* param) {
    uint8_t i = 0;
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SET_CONFIG;
    memcpy(payload + i, param, sizeof(module_param_t));
    i += sizeof(module_param_t);

    lkbt51_send_cmd(payload, i, false);
}

void lkbt51_set_local_name(const char* name) {
    uint8_t i   = 0;
    uint8_t len = strlen(name);
    memset(payload, 0, PACKET_MAX_LEN);

    payload[i++] = LKBT51_CMD_SET_NAME;
    memcpy(payload + i, name, len);
    i += len;
    lkbt51_send_cmd(payload, i, false);
}

static void ack_handler(uint8_t* data) {
    switch (data[1]) {
        case LKBT51_CMD_SEND_KB:
        case LKBT51_CMD_SEND_KB_NKRO:
        case LKBT51_CMD_SEND_CONSUMER:
        case LKBT51_CMD_SEND_SYSTEM:
        case LKBT51_CMD_SEND_MOUSE:
            switch (data[2]) {
                case ACK_SUCCESS:
                    report_buffer_set_retry(0);
                    report_buffer_set_inverval(connection_interval);
                    break;
                case ACK_FIFO_HALF_WARNING:
                    report_buffer_set_retry(0);
                    report_buffer_set_inverval(connection_interval + 5);
                    break;
                case ACK_FIFO_FULL_ERROR:
                    report_buffer_set_inverval(connection_interval + 10);
                    break;
            }
            break;
        default:
            break;
    }
}

static void lkbt51_event_handler(uint8_t evt_type, uint8_t* data) {
    wireless_event_t event = {0};

    switch (evt_type) {
        case LKBT51_EVT_ACK:
            ack_handler(data);
            break;
        case LKBT51_EVT_RESET:
            event.evt_type      = EVT_RESET;
            event.params.reason = data[0];
            break;
        case LKBT51_EVT_HID_EVENT:
            event.evt_type   = EVT_HID_INDICATOR;
            event.params.led = data[0];
            break;
        default:
            break;
    }

    if (event.evt_type) wireless_event_enqueue(event);
}

void lkbt51_task(void) {
#define VALID_DATA_START_INDEX 4
#define BUFFER_SIZE 64

    static bool    wait_for_new_pkt = true;
    static uint8_t len              = 0xff;

    if (gpio_read_pin(WIRELESS_TO_MCU_INT_PIN) == 0) {
        uint8_t buf[BUFFER_SIZE] = {0};
        lkbt51_read(buf, expect_len);

        uint8_t* pbuf = buf + VALID_DATA_START_INDEX;

        if (pbuf[0] == 0xAA && pbuf[1] == 0x54 && pbuf[4] == (uint8_t)(~0x54) && pbuf[5] == (uint8_t)(~0xAA)) {
            /* Module protocol announcement; no action required. */
        } else if (pbuf[0] == 0xAA) {
            wireless_event_t event    = {0};
            uint8_t          evt_mask = pbuf[1];
            lkbt51_last_comm_time     = 0;

            if (evt_mask & LK_EVT_MSK_RESET) {
                event.evt_type      = EVT_RESET;
                event.params.reason = pbuf[2];
                wireless_event_enqueue(event);
            }

            if (evt_mask & LK_EVT_MSK_CONNECTION) {
                lkbt51_send_conn_evt_ack();
                switch (pbuf[2]) {
                    case LKBT51_CONNECTED:
                        event.evt_type = EVT_CONNECTED;
                        break;
                    case LKBT51_DISCOVERABLE:
                        event.evt_type = EVT_DISCOVERABLE;
                        break;
                    case LKBT51_RECONNECTING:
                        event.evt_type = EVT_RECONNECTING;
                        break;
                    case LKBT51_DISCONNECTED:
                        event.evt_type = EVT_DISCONNECTED;
                        break;
                    case LKBT51_PINCODE_ENTRY:
                        event.evt_type = EVT_BT_PINCODE_ENTRY;
                        break;
                    case LKBT51_EXIT_PINCODE_ENTRY:
                        event.evt_type = EVT_EXIT_BT_PINCODE_ENTRY;
                        break;
                    case LKBT51_SLEEP:
                        event.evt_type = EVT_SLEEP;
                        break;
                }
                event.params.hostIndex = pbuf[3];

                wireless_event_enqueue(event);
            }

            if (evt_mask & LK_EVT_MSK_LED) {
                memset(&event, 0, sizeof(event));
                event.evt_type   = EVT_HID_INDICATOR;
                event.params.led = pbuf[4];
                wireless_event_enqueue(event);
            }

            if (evt_mask & LK_EVT_MSK_RPT_INTERVAL) {
                uint32_t interval;
                if (pbuf[8] & 0x80) {
                    interval = (pbuf[8] & 0x7F) * 1250;
                } else {
                    interval = (pbuf[8] & 0x7F) * 125;
                }

                connection_interval = interval / 1000;
                if (connection_interval > 7) connection_interval /= 3;

                memset(&event, 0, sizeof(event));
                event.evt_type        = EVT_CONECTION_INTERVAL;
                event.params.interval = connection_interval;
                wireless_event_enqueue(event);
            }

            if (evt_mask & LK_EVT_MSK_BATT) {
                battery_calculate_voltage(pbuf[6] << 8 | pbuf[5]);
            }
        }

        pbuf = buf;
        if (wait_for_new_pkt) {
            for (uint8_t i = 10; i < BUFFER_SIZE - 5; i++) {
                if (buf[i] == 0xAA && buf[i + 1] == 0x57     // Packet Head
                    && (~buf[i + 2] & 0xFF) == buf[i + 3]) { // Check wheather len is valid
                    len              = buf[i + 2];
                    pbuf             = &buf[i + 5];
                    wait_for_new_pkt = false;
                }
            }
        }

        if (!wait_for_new_pkt && BUFFER_SIZE - 5 >= len) {
            wait_for_new_pkt = true;

            uint16_t checksum = 0;
            for (int i = 0; i < len - 2; i++) {
                checksum += pbuf[i];
            }

            if ((checksum & 0xff) == pbuf[len - 2] && ((checksum >> 8) & 0xff) == pbuf[len - 1]) {
                lkbt51_event_handler(pbuf[0], pbuf + 1);
            } else {
                // TODO: Error handle
            }
        }
    }
    // Check if we need to reset the module
    if (lkbt51_last_comm_time && timer_elapsed32(lkbt51_last_comm_time) > LKBT51_COMM_TIMEOUT_MS) {
        gpio_write_pin_low(LKBT51_RESET_PIN);
        wait_ms(10);
        gpio_write_pin_high(LKBT51_RESET_PIN);
        wait_ms(200);
        lkbt51_connect(0, 0);
        lkbt51_last_comm_time = 0;
    }
}
