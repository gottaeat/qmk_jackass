USE_FPU = yes

ANALOG_MATRIX_DIR = $(TOP_DIR)/keyboards/jackass/mk1/common/analog_matrix
SRC += \
    i2c_master.c \
    $(ANALOG_MATRIX_DIR)/eeprom_he.c \
    $(ANALOG_MATRIX_DIR)/analog_matrix_scan.c \
    $(ANALOG_MATRIX_DIR)/profile.c \
    $(ANALOG_MATRIX_DIR)/action_regular_trigger.c \
    $(ANALOG_MATRIX_DIR)/analog_matrix.c

VPATH += $(ANALOG_MATRIX_DIR)
