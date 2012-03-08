
libs += arduino_core_emulation

arduino_core_emulation_path := $(ARDUINOCORE)
arduino_core_emulation_objs := Print.o Stream.o WString.o
arduino_core_emulation_cflags := -I$(CPLUSPLUS)

