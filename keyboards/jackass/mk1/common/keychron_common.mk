KEYCHRON_COMMON_DIR = $(TOP_DIR)/keyboards/jackass/mk1/common
SRC += \
    $(KEYCHRON_COMMON_DIR)/keychron_task.c \
    $(KEYCHRON_COMMON_DIR)/keychron_common.c

VPATH += $(KEYCHRON_COMMON_DIR)
