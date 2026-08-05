# Increase stack size to avoid crashing in eeprom_update_block().
USE_PROCESS_STACKSIZE = 0x2000
USE_FPU = yes

GRAVE_ESC_ENABLE = no
MAGIC_ENABLE = no
SEND_STRING_ENABLE = no
SPACE_CADET_ENABLE = no

OPT_DEFS += -DLK_WIRELESS_ENABLE
OPT_DEFS += -DWIRELESS_CONFIG_ENABLE
OPT_DEFS += -DNO_USB_STARTUP_CHECK
OPT_DEFS += -DCORTEX_ENABLE_WFI_IDLE=TRUE
OPT_DEFS += -DRGB_MATRIX_SNLED27351_SPI
OPT_DEFS += -DRGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL=48

SPI_DRIVER_REQUIRED = yes

MK1_DRIVER_DIR = $(TOP_DIR)/keyboards/jackass/mk1/drivers
SRC += \
    $(MK1_DRIVER_DIR)/snled27351-spi.c \
    $(MK1_DRIVER_DIR)/rgb_driver.c
VPATH += $(MK1_DRIVER_DIR)

include keyboards/jackass/mk1/common/analog_matrix/analog_matrix.mk
include keyboards/jackass/mk1/common/keychron_common.mk
include keyboards/jackass/mk1/common/wireless/wireless.mk

SRC += board.c debounce.c

OPT = 2
