
# ########################################################################### #
# Toolchain Info
# ########################################################################### #

TC := arm-none-eabi-

CC := $(TC)gcc
CXX := $(TC)g++
LD := $(TC)gcc
AR := $(TC)ar
OBJCOPY := $(TC)objcopy

# ########################################################################### #
# Compiler Flags
# ########################################################################### #

OPTIMIZATION = -Os

CFLAGS := -Wall -mthumb -mcpu=cortex-m3 \
          -mlong-calls -ffunction-sections -g \
          $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) \
          -DTRACE_LEVEL=$(TRACE_LEVEL)
CXXFLAGS := -Wall -mthumb -mcpu=cortex-m3 \
          -mlong-calls -ffunction-sections -g \
          $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) \
          -DTRACE_LEVEL=$(TRACE_LEVEL)

ASFLAGS := -mcpu=cortex-m3 -mthumb -Wall -g \
           $(OPTIMIZATION) $(INCLUDES) \
           -D$(CHIP) -D__ASSEMBLY__

LDFLAGS := -g $(OPTIMIZATION) -nostartfiles -mthumb -mcpu=cortex-m3 \
           -Wl,--gc-sections

LDSCRIPT := $(AT91LIB)/boards/$(BOARD)/$(CHIP)/cxx_flash.ld

