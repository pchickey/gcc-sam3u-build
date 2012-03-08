
libs += arduino_core

arduino_core_path := $(ARDUINOCORE)
arduino_core_objs := Print.o Stream.o WString.o itoa.o
arduino_core_cflags := -I$(CPLUSPLUS)

