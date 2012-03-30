
# Include defaults.mk first in each Makefile
include ../defaults.mk

targets += simple

simple_objs := simple.o vector_table.o
simple_libs := at91lib_board at91lib_peripherals at91lib_utility
simple_cflags := -std=c99 -I$(AT91LIB) \
            -I$(AT91LIB)/peripherals \
            -I$(AT91LIB)/boards/$(BOARD) \

default: simple

# Include rules.mk last in each Makefile
include ../rules.mk

