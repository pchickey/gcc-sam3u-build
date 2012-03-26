
TOP := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
LIBDIR := $(TOP)/libs

CHIP := at91sam3u4
BOARD := at91sam3u-ek

AT91LIB  := $(TOP)/at91lib
TRACE_LEVEL := 4
FREERTOS := $(TOP)/freertos
FREERTOS_PORT := $(FREERTOS)/portable/GCC/ARM_CM3
CMSIS := $(TOP)/cmsis
ARDUINOCORE := $(TOP)/arduino-core
CPLUSPLUS := $(TOP)/cplusplus
SYSCALLS := $(TOP)/syscalls
