WIRELESS_DIR = $(TOP_DIR)/keyboards/jackass/mk1/common/wireless
SRC += \
    $(WIRELESS_DIR)/wireless.c \
    $(WIRELESS_DIR)/report_buffer.c \
    $(WIRELESS_DIR)/lkbt51.c \
    $(WIRELESS_DIR)/indicator.c \
    $(WIRELESS_DIR)/wireless_main.c \
    $(WIRELESS_DIR)/transport.c \
    $(WIRELESS_DIR)/lpm.c \
    $(WIRELESS_DIR)/battery.c \
    $(WIRELESS_DIR)/battery_indicator.c \
    $(WIRELESS_DIR)/rtc_timer.c \
    $(WIRELESS_DIR)/keychron_wireless_common.c \
    $(WIRELESS_DIR)/lpm_stm32f401.c

VPATH += $(WIRELESS_DIR)
