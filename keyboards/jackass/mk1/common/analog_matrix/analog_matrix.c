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

#include "quantum.h"
#include "analog_matrix.h"
#include "eeprom.h"
#include "eeprom_he.h"
#include "profile.h"
#include "nvm_eeprom_eeconfig_internal.h"

#define CAL_SAMPL_CNT 8
#define REF_ZERO_TRAVEL 3121

#define CONST_A1 426.88962
#define CONST_B1 -0.48358
#define CONST_C1 2.04637e-4
#define CONST_D1 -2.99368e-8

#define TRAVEL_POLYNOMIAL(x) (CONST_A1 + CONST_B1 * x + CONST_C1 * x * x + CONST_D1 * x * x * x)

STATIC_ASSERT(sizeof(calibrated_value_t) == SIZE_OF_CALIB_VALUE_T, "Calibration EEPROM layout changed");

enum {
    CALIB_OFF = 0,
    CALIB_ZERO_TRAVEL_POWER_ON,
};

enum {
    CALI_ZERO_TRAVEL = 0x01 << 0,
};

enum {
    AUTO_CALIB_OFF = 0,
    AUTO_CALIB_NEXT_LOOP,
    AUTO_CALIB_ZERO_TRAVEL,
    AUTO_CALIB_FULL_TRAVEL,
    AUTO_CALIB_FINISHED,
};

extern const matrix_row_t analog_matrix_mask[];
extern bool               regular_trigger_action(analog_key_t *key);

static calibrated_value_t calib_values[MATRIX_ROWS][MATRIX_COLS];
static calibrated_value_t saved_calib_values[MATRIX_ROWS][MATRIX_COLS];
static analog_key_t       analog_key_matrix[MATRIX_ROWS][MATRIX_COLS];

static uint16_t      calibrate_values[MATRIX_ROWS][MATRIX_COLS][CAL_SAMPL_CNT];
static calibration_t auto_calib[MATRIX_ROWS][MATRIX_COLS];
static uint8_t       cali_state = CALIB_OFF;
static uint8_t       cur_calib  = 0;
static float         scale_factor[MATRIX_ROWS][MATRIX_COLS];

static uint8_t calibrated;

static uint8_t convert_to_travel(uint8_t row, uint8_t col, uint16_t value) {
    uint16_t travel;

    calibrated_value_t *p_calib = &calib_values[row][col];

    int16_t  offset = p_calib->zero_travel - REF_ZERO_TRAVEL;
    uint16_t x      = value - offset;

    if (x > REF_ZERO_TRAVEL) return 0;

    travel = (uint16_t)((TRAVEL_POLYNOMIAL(x) - TRAVEL_POLYNOMIAL(REF_ZERO_TRAVEL)) * scale_factor[row][col] * TRAVEL_SCALE + 0.5);
    if (travel > (FULL_TRAVEL_UNIT + 1) * TRAVEL_SCALE - 1) travel = (FULL_TRAVEL_UNIT + 1) * TRAVEL_SCALE - 1;

    return travel & 0xFF;
}

static void update_key_config(uint8_t row, uint8_t col) {
    analog_key_t *p_key           = &analog_key_matrix[row][col];
    uint8_t       actuation_point = profile_get_actuation_point();

    p_key->regular.actn_pt   = actuation_point * TRAVEL_SCALE;
    p_key->regular.deactn_pt = (actuation_point - 3) * TRAVEL_SCALE;
}

void update_travel_configs(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            update_key_config(r, c);
        }
    }
}

static void update_default_travel(void) {
    // clang-format off
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if ((analog_matrix_mask[r] & (0x01<<c)) == 0) continue;

            if (saved_calib_values[r][c].zero_travel == 0)
                saved_calib_values[r][c].zero_travel = DEFAULT_ZERO_TRAVEL_VALUE;

            if (saved_calib_values[r][c].full_travel == 0)
                saved_calib_values[r][c].full_travel = saved_calib_values[r][c].zero_travel - DEFAULT_FULL_RANGE;
        }
    }
    // clang-format off
}

static inline void update_scale_factor(uint8_t row, uint8_t col) {
    calibrated_value_t *p_calib = &calib_values[row][col];

    if (calibrated) {
        int16_t offset      = p_calib->zero_travel - REF_ZERO_TRAVEL;
        uint16_t x          = p_calib->full_travel - offset;

        float    full_travel = (TRAVEL_POLYNOMIAL(x) - TRAVEL_POLYNOMIAL(REF_ZERO_TRAVEL));
        scale_factor[row][col]   = FULL_TRAVEL_UNIT / full_travel;
    } else
        scale_factor[row][col] = 1.0f;

}

static void update_scale_factors(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            update_scale_factor(r, c);

        }
    }
}

void analog_matrix_eeprom_update(const void *buf, void *addr, size_t len) {
    addr += EECONFIG_BASE_ANALOG_MATRIX;
    eeprom_update_block(buf, addr, len);
}

static void save_calibration_value(uint8_t row, uint8_t col) {
    static uint8_t eeprom_calibrated;

    if (eeprom_calibrated != calibrated) {
        eeprom_calibrated = calibrated;
        he_eeprom_write_block(&calibrated, (void *)(EXTERNAL_EEPROM_OFFSET + OFFSET_CALIBRATION), 1);
    }

    uint32_t offset = OFFSET_CALIBRATED_DATA_START + (&saved_calib_values[row][col]-&saved_calib_values[0][0])* sizeof(saved_calib_values[0][0]);
    he_eeprom_write_block(&saved_calib_values[row][col], (void *)(EXTERNAL_EEPROM_OFFSET + offset), sizeof(saved_calib_values[0][0]));

    // Save a copy to emulated EEPROM
    if (!eeconfig_is_kb_datablock_valid()) eeprom_update_dword(EECONFIG_KEYBOARD, (EECONFIG_KB_DATA_VERSION));

    analog_matrix_eeprom_update(&calibrated, (void *)OFFSET_CALIBRATION, 1);
    analog_matrix_eeprom_update(&saved_calib_values[row][col], (void *)offset, sizeof(saved_calib_values[0][0]));
}

static void save_calibration_values(void) {
    // Save to external EEPROM
    static uint8_t eeprom_calibrated;

    if (eeprom_calibrated != calibrated) {
        eeprom_calibrated = calibrated;
        he_eeprom_write_block(&calibrated, (void *)(EXTERNAL_EEPROM_OFFSET + OFFSET_CALIBRATION), 1);
    }

    he_eeprom_write_block(saved_calib_values, (void *)(EXTERNAL_EEPROM_OFFSET + OFFSET_CALIBRATED_DATA_START), sizeof(saved_calib_values));

    // Save a copy to emulate EEPROM
    if (!eeconfig_is_kb_datablock_valid()) eeprom_update_dword(EECONFIG_KEYBOARD, (EECONFIG_KB_DATA_VERSION));

    analog_matrix_eeprom_update(&calibrated, OFFSET_CALIBRATION, 1);
    analog_matrix_eeprom_update(saved_calib_values, (uint8_t *)OFFSET_CALIBRATED_DATA_START, sizeof(saved_calib_values));

    update_default_travel();
    update_travel_configs();
    update_scale_factors();
}

static bool calibrate(void) {
    if (cali_state == CALIB_OFF) return false;

    if (cur_calib + 1 < CAL_SAMPL_CNT) {
        ++cur_calib;
        return false;
    }

    bool valid = true;

    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if ((analog_matrix_mask[r] & (0x01 << c)) == 0) continue;

            uint32_t sum = 0;
            for (uint8_t i = 0; i < CAL_SAMPL_CNT; i++) {
                sum += calibrate_values[r][c][i];
            }

            uint16_t avg_val = sum / CAL_SAMPL_CNT;
            if (avg_val > VALID_ANALOG_RAW_VALUE_MAX || avg_val < DEFAULT_ZERO_TRAVEL_VALUE - DEFAULT_FULL_RANGE / 5) {
                valid = false;
                if (avg_val < DEFAULT_ZERO_TRAVEL_VALUE - DEFAULT_FULL_RANGE / 5) {
                    auto_calib[r][c].pressed = true;
                }
                continue;
            }

            avg_val -= ZERO_TRAVEL_DEAD_ZONE;
            if (abs(avg_val - calib_values[r][c].zero_travel) > 30) {
                calib_values[r][c].zero_travel = avg_val;
                calib_values[r][c].full_travel = avg_val - DEFAULT_FULL_RANGE;
            }
        }
    }

    if (valid && (calibrated & CALI_ZERO_TRAVEL) == 0) {
        for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                saved_calib_values[r][c] = calib_values[r][c];
            }
        }
        calibrated |= CALI_ZERO_TRAVEL;
        save_calibration_values();
    }

    cali_state = CALIB_OFF;
    return true;
}

static void calibration_validate(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++)
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if ((analog_matrix_mask[r] & (0x01 << c)) == 0) continue;

            if (saved_calib_values[r][c].zero_travel < DEFAULT_ZERO_TRAVEL_VALUE - DEFAULT_FULL_RANGE / 5) {
                saved_calib_values[r][c].zero_travel = DEFAULT_ZERO_TRAVEL_VALUE;
                saved_calib_values[r][c].full_travel = saved_calib_values[r][c].zero_travel - DEFAULT_FULL_RANGE;
            }
        }
}

static void auto_calibration_init(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++)
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            memset(&auto_calib[r][c], 0, sizeof(auto_calib[0][0]));
        }
}

static void auto_caliration_check(uint8_t row, uint8_t col, uint16_t value) {
    calibration_t *p = &auto_calib[row][col];

    switch (p->state) {
        case AUTO_CALIB_OFF:
            if (p->pressed) {
                if (value > DEFAULT_ZERO_TRAVEL_VALUE - DEFAULT_FULL_RANGE / 5) {
                    p->pressed = false;
                } else
                    return;
            }
            p->state      = AUTO_CALIB_NEXT_LOOP; // fall through
            p->confidence = 0;
            break;

        case AUTO_CALIB_NEXT_LOOP:
            if (value < AUTO_CALIB_FULL_TRAVEL_THRESHOLD_VALUE) {
                p->value.full_travel = value;
                p->state             = AUTO_CALIB_FULL_TRAVEL;
                p->full_travel_time  = timer_read32();
            }
            break;

        case AUTO_CALIB_ZERO_TRAVEL:
            if (value > p->value.zero_travel) {
                // Key continues releasing
                p->value.zero_travel = value;
                p->cycle             = 0;
            } else if (abs(p->value.zero_travel - value) <= AUTO_CALIB_ZERO_TRAVEL_JITTER_VALUE) {
                //  Add the data to buffer
                calibrate_values[row][col][p->cycle++] = value;
                if (p->cycle >= CAL_SAMPL_CNT) {
                    if (timer_elapsed32(p->full_travel_time) < AUTO_CALIB_VALID_RELASING_TIME) {
                        uint32_t avg_val = 0;
                        for (uint8_t i = 0; i < CAL_SAMPL_CNT; i++)
                            avg_val += calibrate_values[row][col][i];

                        avg_val /= CAL_SAMPL_CNT;
                        avg_val -= ZERO_TRAVEL_DEAD_ZONE;

                        p->value.zero_travel = avg_val;

                        // Update confidence
                        if (avg_val - p->value.full_travel > 1100) {
                            p->confidence += 6;
                        } else if (avg_val - p->value.full_travel > 1000) {
                            p->confidence += 3;
                        } else if (avg_val - p->value.full_travel > 900) {
                            p->confidence += 2;
                        }
                    }

                    p->state = AUTO_CALIB_NEXT_LOOP;
                }
            } else {
                p->state = AUTO_CALIB_NEXT_LOOP;
            }

            if (p->confidence >= 12) {
                p->state = AUTO_CALIB_FINISHED;

                if (abs(p->value.zero_travel - calib_values[row][col].zero_travel) > 10 || abs(p->value.full_travel + BOTTOM_JITTER - calib_values[row][col].full_travel) > 30) {
                    calib_values[row][col].zero_travel = p->value.zero_travel;
                    calib_values[row][col].full_travel = p->value.full_travel + BOTTOM_JITTER;

                    if (abs(saved_calib_values[row][col].zero_travel - calib_values[row][col].zero_travel) > 15 || abs(saved_calib_values[row][col].full_travel - calib_values[row][col].full_travel) > 50) {
                        /* Save */
                        saved_calib_values[row][col].zero_travel = calib_values[row][col].zero_travel;
                        saved_calib_values[row][col].full_travel = calib_values[row][col].full_travel;
                        save_calibration_value(row, col);
                    } else {
                        calib_values[row][col].full_travel = p->value.full_travel - 15;
                    }

                    update_default_travel();
                    update_travel_configs();
                    update_scale_factors();
                }
            }
            break;

        case AUTO_CALIB_FULL_TRAVEL:
            if (value < p->value.full_travel) {
                // Key continues pressing
                p->value.full_travel = value;
                p->full_travel_time  = timer_read32();
            } else if (value - p->value.full_travel <= AUTO_CALIB_FULL_TRAVEL_JITTER_VALUE) {
                // Key is still pressed, we encounter system noise value jitter, just update the time
                p->full_travel_time = timer_read32();
            } else if (value - p->value.full_travel > AUTO_CALIB_ZERO_TRAVEL_THRESHOLD_VALUE) {
                // Key releasing detected
                p->value.zero_travel = value;
                p->state             = AUTO_CALIB_ZERO_TRAVEL;
                p->cycle             = 0;
            }
            break;

        case AUTO_CALIB_FINISHED:
            if (value > p->value.zero_travel + 50) p->state = AUTO_CALIB_OFF;
            break;

        default:
            break;
    }
}

static void analog_matrix_eeconfig_init(void) {
    bool reset_profiles = false;
    if (!eeconfig_is_enabled()) {
        eeconfig_init();
        reset_profiles = true;
    } else if (!eeconfig_is_kb_datablock_valid()) {
        eeconfig_init_kb_datablock();
        reset_profiles = true;
    }

    profile_init(reset_profiles);

    uint8_t *buf = (uint8_t *)malloc(EECONFIG_SIZE_ANALOG_MATRIX);
    memset(buf, 0, EECONFIG_SIZE_ANALOG_MATRIX);

    eeprom_read_block(buf, (void *)EECONFIG_BASE_ANALOG_MATRIX, EECONFIG_SIZE_ANALOG_MATRIX);

    // Load calibration data
    calibrated = buf[OFFSET_CALIBRATION];
    memset(calib_values, 0, sizeof(calib_values));
    memset(saved_calib_values, 0, sizeof(saved_calib_values));

    if (calibrated) {
        memcpy(saved_calib_values, buf + OFFSET_CALIBRATED_DATA_START, sizeof(saved_calib_values));
    } else {
        uint32_t buf;
        he_eeprom_read_block(&buf, 0, 4); // Magic number

        if (buf != VENDOR_ID) {
            he_eeprom_driver_erase();
            buf = VENDOR_ID;
            he_eeprom_write_block(&buf, 0, 4);
        } else {
            he_eeprom_read_block(&calibrated, (void *)(EXTERNAL_EEPROM_OFFSET + OFFSET_CALIBRATION), 1);
            if (calibrated) {
                he_eeprom_read_block(saved_calib_values, (void *)(EXTERNAL_EEPROM_OFFSET + OFFSET_CALIBRATED_DATA_START), sizeof(calib_values));
                // Save to emulated EEPROM
                analog_matrix_eeprom_update(&calibrated, OFFSET_CALIBRATION, 1);
                analog_matrix_eeprom_update(saved_calib_values, (uint8_t *)OFFSET_CALIBRATED_DATA_START, sizeof(saved_calib_values));
            }
        }
    }

    // Reset to default if calibrated data is invalid
    for (uint8_t r = 0; r < MATRIX_ROWS; r++)
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if ((analog_matrix_mask[r] & (0x01U << c)) == 0) continue;
            if (saved_calib_values[r][c].zero_travel > VALID_ANALOG_RAW_VALUE_MAX || saved_calib_values[r][c].zero_travel < DEFAULT_ZERO_TRAVEL_VALUE - 300) saved_calib_values[r][c].zero_travel = DEFAULT_ZERO_TRAVEL_VALUE;

            if (saved_calib_values[r][c].full_travel > DEFAULT_FULL_TRAVEL_VALUE + 400 || saved_calib_values[r][c].full_travel < VALID_ANALOG_RAW_VALUE_MIN) saved_calib_values[r][c].full_travel = saved_calib_values[r][c].zero_travel - DEFAULT_FULL_RANGE;
        }

    if (calibrated) {
        calibration_validate();
        update_default_travel();
    }
    memcpy(calib_values, saved_calib_values, sizeof(saved_calib_values));

    // Load actuation/deacuation dat
    memset(analog_key_matrix, 0, sizeof(analog_key_matrix));
    update_travel_configs();
    update_scale_factors();

    auto_calibration_init();
    cali_state = CALIB_ZERO_TRAVEL_POWER_ON;

    free(buf);
}

void analog_matrix_init(void) {
    he_eeprom_driver_init();

    analog_matrix_eeconfig_init();

    cur_calib = 0;
    memset(calibrate_values, 0, MATRIX_ROWS * MATRIX_COLS * CAL_SAMPL_CNT * sizeof(calibrate_values[0][0][0]));

    // 解决 boot magic 扫描无效, TODO: 这里会增加启动时间
    for (uint8_t i = 0; i < CAL_SAMPL_CNT; i++)
        matrix_scan();
}

bool update_raw_value(uint8_t row, uint8_t col, uint16_t value) {
    if (value < VALID_ANALOG_RAW_VALUE_MIN || value > VALID_ANALOG_RAW_VALUE_MAX) return false;

    if (cali_state) {
        calibrate_values[row][col][cur_calib] = value;
        return false;
    }
    auto_caliration_check(row, col, value);

    analog_key_t *k = &analog_key_matrix[row][col];

    if (abs(k->last_val - value) < 5) return false;

    k->last_val = value;
    k->travel   = convert_to_travel(row, col, value);

    if (k->travel == k->last_travel) return false;

    k->last_travel = k->travel;

    return regular_trigger_action(k);
}

bool analog_matrix_get_key_state(uint8_t row, uint8_t col) {
    analog_key_t *k = &analog_key_matrix[row][col];
    return k->state == AKS_REGULAR_PRESSED;
}

void analog_matrix_task(void) {
    calibrate();
    profile_indication_timer_check();
}

void analog_matrix_clear(void) {
    memset(analog_key_matrix, 0, sizeof(analog_key_matrix));
}
