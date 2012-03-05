
libs += at91lib_board at91lib_peripherals at91lib_utility

at91lib_board_path := $(AT91LIB)/boards/$(BOARD)
at91lib_board_objs := board_cstartup_gnu.o \
                      board_lowlevel.o \
                      board_memories.o \
                      exceptions.o
at91lib_board_cflags := -I$(AT91LIB)/boards/$(BOARD) \
												-I$(AT91LIB)/peripherals
												

at91lib_peripherals_path := $(AT91LIB)/peripherals
at91lib_peripherals_objs := adc/adc.o \
                            adc/adc12.o \
                            chipid/chipid.o \
                            dbgu/dbgu.o \
                            dma/dma.o \
                            eefc/eefc.o \
                            hsmc4/hsmc4.o \
                            hsmc4/hsmc4_ecc.o \
                            irq/nvic.o \
                            lcd/lcd.o \
                            mci/mci.o \
                            pio/pio.o \
                            pio/pio_it.o \
                            pwmc/pwmc2.o \
                            rstc/rstc.o \
                            rtc/rtc.o \
                            rtt/rtt.o \
                            slck/slck.o \
                            spi/spi.o \
                            systick/systick.o \
                            tc/tc.o \
                            tsadcc/tsadcc.o \
                            twi/twi.o \
                            usart/usart.o
at91lib_peripherals_cflags := -I$(AT91LIB)/boards/$(BOARD) \
															-I$(AT91LIB)/peripherals \
															-I$(TOP) \
															-I$(AT91LIB)

at91lib_utility_path := $(AT91LIB)
at91lib_utility_objs := $(addprefix utility/,\
													bmp.o \
													clock.o \
													hamming.o \
													led.o \
													math.o \
													rand.o \
													retarget.o \
													stdio.o \
													string.o \
													trace.o \
													video.o \
													wav.o)
at91lib_utility_cflags := -I$(AT91LIB)/boards/$(BOARD) \
													-I$(AT91LIB)/peripherals \
													-I$(AT91LIB)

