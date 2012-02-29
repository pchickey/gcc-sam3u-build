
# ########################################################################### #
# at91 chip and board names (used in at91lib)
# ########################################################################### #

CHIP := at91sam3u4
BOARD := at91sam3u-ek

# ########################################################################### #
# at91lib directories and source files
# ########################################################################### #

AT91LIB :=../at91lib
TRACE_LEVEL := 4

# ########################################################################### #
# Toolchain Info
# ########################################################################### #

TC := arm-none-eabi-

CC := $(TC)gcc
LD := $(TC)gcc
AR := $(TC)ar

# ########################################################################### #
# Compiler Flags
# ########################################################################### #

INCLUDES := -I$(AT91LIB) \
            -I$(AT91LIB)/peripherals \
            -I$(AT91LIB)/boards/$(BOARD)

OPTIMIZATION = -Os

CFLAGS := -std=c99 -Wall -mthumb -mcpu=cortex-m3 \
          -mlong-calls -ffunction-sections -g \
          $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) \
          -DTRACE_LEVEL=$(TRACE_LEVEL)

ASFLAGS := -mcpu=cortex-m3 -mthumb -Wall -g \
           $(OPTIMIZATION) $(INCLUDES) \
           -D$(CHIP) -D__ASSEMBLY__

LDFLAGS := -g $(OPTIMIZATION) -nostartfiles -mthumb -mcpu=cortex-m3 \
           -Wl,--gc-sections

LDSCRIPT := $(AT91LIB)/boards/$(BOARD)/$(CHIP)/flash.lds

# ########################################################################### #
# Rules
# ########################################################################### #

targets += simple

simple_objs := simple.o
simple_libs := at91lib_board at91lib_peripherals at91lib_utility

# ########################################################################### #
# Rules
# ########################################################################### #


default: simple.elf

include ../rules.mk

