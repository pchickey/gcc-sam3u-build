
libs += freertos_port_cm3 freertos_src

freertos_port_cm3_path := $(FREERTOS)/portable/GCC/ARM_CM3
freertos_port_cm3_objs := port.o
freertos_port_cm3_cflags := -I$(FREERTOS)/include \
														-I$(FREERTOS)/portable/GCC/ARM_CM3

freertos_src_path := $(FREERTOS)
freertos_src_objs := croutine.o list.o queue.o tasks.o timers.o
freertos_src_cflags := -I$(FREERTOS)/include \
											 -I$(FREERTOS)/portable/GCC/ARM_CM3

