/* Copyright 2024 @ Keychron (https://www.keychron.com)
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

#include "stdint.h"
#include "hal.h"
#include "gpio.h"
#include "quantum.h"
#include "analog_matrix.h"
#include "lpm.h"

#define HC164_DS B3
#define HC164_CP B5
#define HC164_MR D2

#define ADC_GRP_NUM_CHANNELS MATRIX_ROWS
#define ADC_GRP_BUF_DEPTH 1
#define UNUSED_DEPTH 0

static matrix_row_t analog_raw_matrix[MATRIX_ROWS];
static pin_t        row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static bool         matrix_changed;

static adcsample_t samples[ADC_GRP_NUM_CHANNELS * ADC_GRP_BUF_DEPTH];

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
    dprintf("err\r\n");

    (void)adcp;
    (void)err;
}

// clang-format off
ADCConversionGroup adcgrpcfg = {
    FALSE,
    6,
    NULL,
    adcerrorcallback,
    0,                                          /* CR1 */
    ADC_CR2_SWSTART,                            /* CR2 */
    0,                                          /* SMPR1 */
    0,                                          /* SMPR2 */
    0,                                          /* HTR */
    0,                                          /* LTR */
    0,                                          /* SQR1 */
    0,                                          /* SQR2 */
    0                                           /* SQR3 */
};
// clang-format on

static uint8_t pinToAdcChn(pin_t pin) {
    switch (pin) {
        case A0:
            return ADC_CHANNEL_IN0;
        case A1:
            return ADC_CHANNEL_IN1;
        case A2:
            return ADC_CHANNEL_IN2;
        case A3:
            return ADC_CHANNEL_IN3;
        case A4:
            return ADC_CHANNEL_IN4;
        case A5:
            return ADC_CHANNEL_IN5;
        case A6:
            return ADC_CHANNEL_IN6;
        case A7:
            return ADC_CHANNEL_IN7;
        case B0:
            return ADC_CHANNEL_IN8;
        case B1:
            return ADC_CHANNEL_IN9;
        case C0:
            return ADC_CHANNEL_IN10;
        case C1:
            return ADC_CHANNEL_IN11;
        case C2:
            return ADC_CHANNEL_IN12;
        case C3:
            return ADC_CHANNEL_IN13;
        case C4:
            return ADC_CHANNEL_IN14;
        case C5:
            return ADC_CHANNEL_IN15;
    }

    return 0xFF;
}

static inline void shifter_delay(uint16_t n) {
    while (n-- > 0) {
        asm volatile("nop" ::: "memory");
    }
}

static void HC164_output(uint16_t data, bool bit_flag) {
    uint8_t n = 50;

    ATOMIC_BLOCK_FORCEON {
        for (uint8_t i = 0; i < 15; i++) {
            if (data & 0x1) {
                gpio_write_pin_high(HC164_DS);
            } else {
                gpio_write_pin_low(HC164_DS);
            }
            shifter_delay(n);
            gpio_write_pin_high(HC164_CP);
            shifter_delay(n);
            gpio_write_pin_low(HC164_CP);
            shifter_delay(n);
            if (bit_flag) {
                break;
            } else {
                data = data >> 1;
            }
        }
    }
}

static bool select_col(uint8_t col) {
    if (col == 0) {
        gpio_write_pin_low(HC164_MR);
        shifter_delay(20);
        gpio_write_pin_high(HC164_MR);
        shifter_delay(20);
        HC164_output(0x01, true);
    }
    return true;
}

static void unselect_col(uint8_t col) {
    HC164_output(0x00, true);
    return;
}

static void matrix_read_rows_on_col(uint8_t current_col, matrix_row_t row_shifter) {
    // Select col
    if (!select_col(current_col)) {
        return; // skip NO_PIN col
    }

    wait_us(40);

    uint8_t debouce_times = ANALOG_DEBOUCE_TIME;
    uint8_t row_value     = 0;
    bool    changed       = false;

    do {
        adcConvert(&ADCD1, &adcgrpcfg, samples, ADC_GRP_BUF_DEPTH);

        uint8_t row_value_recheck = 0;
        for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
            matrix_row_t row_mask = 0x01 << current_col;
            update_raw_value(row_index, current_col, samples[row_index]);

            bool pressed = analog_matrix_get_key_state(row_index, current_col);
            if (pressed) {
                if ((analog_raw_matrix[row_index] & row_mask) == 0) changed = true;

                if (debouce_times == ANALOG_DEBOUCE_TIME) {
                    row_value |= (0x01 << row_index);
                } else {
                    row_value_recheck |= (0x01 << row_index);
                }
            } else if (analog_raw_matrix[row_index] & row_mask) {
                changed = true;
            }
        }

        if (debouce_times != ANALOG_DEBOUCE_TIME && row_value != row_value_recheck) {
            // Clear state when bounce occurs
            changed = false;
        }

    } while (--debouce_times && changed);

    if (changed) {
        matrix_changed = true;
        for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
            if (row_value & (0x01 << row_index)) {
                analog_raw_matrix[row_index] |= row_shifter;
            } else {
                analog_raw_matrix[row_index] &= ~row_shifter;
            }
        }
    }

    // Unselect col
    unselect_col(current_col);
}

void matrix_init_custom(void) {
    uint32_t smpr[2] = {0, 0};
    uint32_t sqr[3]  = {0, 0, 0};
    uint8_t  chn;
    uint8_t  chn_cnt = 0;

    gpio_set_pin_output(ANALOG_MATRIX_POWER_PIN);
    gpio_write_pin(ANALOG_MATRIX_POWER_PIN, ANALOG_MATRIX_POWER_ENABLE_LEVEL);
    gpio_set_pin_input_high(ANALOG_MATRIX_WAKEUP_PIN);

    // Init shift register control pins
    gpio_set_pin_output(HC164_DS);
    gpio_set_pin_output(HC164_CP);
    gpio_set_pin_output(HC164_MR);
    gpio_write_pin_low(HC164_MR);

    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        if (row_pins[x] != NO_PIN) {
            palSetLineMode(row_pins[x], PAL_MODE_INPUT_ANALOG);
            palWriteLine(row_pins[x], 0);
        }

        chn = pinToAdcChn(row_pins[x]);
        if (chn < 0xFF) {
            if (chn > 9)
                smpr[0] |= ADC_SAMPLE_56 << ((chn - 10) * 3);
            else
                smpr[1] |= ADC_SAMPLE_56 << (chn * 3);

            sqr[chn_cnt / 6] |= chn << ((chn_cnt % 6) * 5);
            chn_cnt++;
        }
    }

    adcgrpcfg.smpr1 = smpr[0];
    adcgrpcfg.smpr2 = smpr[1];

    adcgrpcfg.sqr3 = sqr[0];
    adcgrpcfg.sqr2 = sqr[1];
    adcgrpcfg.sqr1 = sqr[2];

    adcStart(&ADCD1, NULL);

    // Refer to STM32 AN4073 Option 2
    SYSCFG->PMC |= SYSCFG_PMC_ADC1DC2;

    // Value of initial ADC seems abnormal, scan to skip/drop i
    for (uint8_t i = 0; i < 5; i++)
        for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++) {
            matrix_read_rows_on_col(current_col, 0);
        }

    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        analog_raw_matrix[i] = 0;
    }

    analog_matrix_init();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    matrix_row_t last_raw_matrix[MATRIX_ROWS];

    memcpy(last_raw_matrix, current_matrix, sizeof(last_raw_matrix));
    matrix_changed = false;

    // Set col, read rows
    matrix_row_t row_shifter = MATRIX_ROW_SHIFTER;
    for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++, row_shifter <<= 1) {
        matrix_read_rows_on_col(current_col, row_shifter);
    }

    analog_matrix_task();
    extern const matrix_row_t analog_matrix_mask[];
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = analog_raw_matrix[row] & analog_matrix_mask[row];
    }

    bool changed = memcmp(current_matrix, last_raw_matrix, sizeof(last_raw_matrix)) != 0;

    return matrix_changed | changed;
}

void matrix_enter_low_power(void) {
    adcStop(&ADCD1);

    gpio_set_pin_input_low(HC164_DS);
    gpio_set_pin_input_low(HC164_CP);
    gpio_set_pin_input_low(HC164_MR);
    gpio_write_pin(ANALOG_MATRIX_POWER_PIN, !ANALOG_MATRIX_POWER_ENABLE_LEVEL);
    palEnableLineEvent(ANALOG_MATRIX_WAKEUP_PIN, PAL_EVENT_MODE_FALLING_EDGE);

    // Set all row to input low
    pin_t pins_row[MATRIX_ROWS] = MATRIX_ROW_PINS;
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        if (pins_row[x] != NO_PIN) {
            gpio_set_pin_input_low(pins_row[x]);
        }
    }
}

void matrix_exit_low_power(void) {
    palDisableLineEvent(ANALOG_MATRIX_WAKEUP_PIN);
}
