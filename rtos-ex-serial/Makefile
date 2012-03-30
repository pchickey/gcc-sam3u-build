
include ../defaults.mk

targets += rtos_serial

rtos_serial_objs := rtos_serial.o vector_table.o
rtos_serial_libs := at91lib_board at91lib_peripherals at91lib_utility \
										freertos_port_cm3 freertos_src \
										arduino_core cplusplus \
										freertos_serial \
										syscalls
rtos_serial_cflags := -std=c99 -I$(AT91LIB) \
            -I$(AT91LIB)/peripherals \
            -I$(AT91LIB)/boards/$(BOARD) \
						-I$(FREERTOS)/include \
						-I$(FREERTOS_PORT) \
						-I$(CPLUSPLUS) \
						-I$(FREERTOS)/serial

default: rtos_serial.elf

include ../rules.mk

