
include ../rules.mk

# ########################################################################### #
# at91 chip and board names (used in at91lib)
# ########################################################################### #

CHIP := at91sam3u4
BOARD := at91sam3u-ek

# ########################################################################### #
# at91lib directories and source files
# ########################################################################### #

AT91LIB := ../at91lib

# TRACE_LEVEL used inside at91lib for debugging
TRACE_LEVEL := 4

at91lib_board_path := $(AT91LIB)/boards/$(BOARD)
at91lib_board_objs := board_cstartup_gnu.o \
								   		board_lowlevel.o \
								    	board_memories.o \
								 			exceptions.o

at91lib_peripherals_path := $(AT91LIB)/peripherals
at91lib_peripherals_objs := pio/pio.o \
														usart/usart.o

at91lib_utility_path := $(AT91LIB)/utility
at91lib_utility_objs := trace.o stdio.o


# ########################################################################### #
# Toolchain Info
# ########################################################################### #

TC := arm-none-eabi-

CC := $(TC)gcc
LD := $(TC)gcc

INCLUDES := -I$(AT91LIB) \
						-I$(AT91LIB)/peripherals \
						-I$(AT91LIB)/boards/$(BOARD)

OPTIMIZATION = -Os

CFLAGS := -std=c99 -Wall -mthumb -mcpu=cortex-m3 \
				-mlong-calls -ffunction-sections -g \
				$(OPTIMIZATION)	$(INCLUDES) -D$(CHIP) \
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

simple_libs := at91lib_board at91lib_peripherals at91lib_utility
# eventaully, the following should work:
# $(call gen-target,simple)
# but right now my make macro foo is lacking

simple_objs := simple.o

$(call gen-target-dir-objects,simple,at91lib_board)
$(call gen-target-dir-objects,simple,at91lib_peripherals)
$(call gen-target-dir-objects,simple,at91lib_utility)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

simple.elf: $(simple_objs)
	$(LD) $(LDFLAGS) -T $(LDSCRIPT) -o $@ $^ $(LIBS)

.PHONY:clean
clean:
	$(v)-rm *.o
	$(v)-rm $(simple_objs)
	$(v)-rm simple.elf

