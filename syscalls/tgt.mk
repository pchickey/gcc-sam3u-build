
libs += syscalls

syscalls_path   := $(SYSCALLS)
syscalls_objs   := syscalls_sam3.o rtos_heap.o
syscalls_cflags := -I$(SYSCALLS)


