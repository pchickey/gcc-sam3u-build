
libs += freertos_serial
freertos_serial_path := $(FREERTOS)/serial
freertos_serial_objs := BetterStream.o RTOSSerial.o
freertos_serial_cflags := \
	-I$(FREERTOS)/serial \
	-I$(FREERTOS)/include \
	-I$(FREERTOS_PORT) \
	-I$(CPLUSPLUS) \
	-I$(ARDUINOCORE)

