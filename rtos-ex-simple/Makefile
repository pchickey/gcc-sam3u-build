
include ../defaults.mk

targets += rtos_simple

rtos_simple_objs := rtos_simple.o rtos_cstartup_gnu.o rtos_heap.o
rtos_simple_libs := at91lib_board at91lib_peripherals at91lib_utility \
										freertos_port_cm3 freertos_src cmsis
rtos_simple_cflags := -std=c99 -I$(AT91LIB) \
            -I$(AT91LIB)/peripherals \
            -I$(AT91LIB)/boards/$(BOARD) \
						-I$(FREERTOS)/include \
						-I$(FREERTOS)/portable/GCC/ARM_CM3

default: rtos_simple

include ../rules.mk

