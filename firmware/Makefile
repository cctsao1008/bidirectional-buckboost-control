PROJECT      := buckboost_gate1
DEVICE       := stm32f334c8t6
OPENCM3_DIR  ?= ../third_party/libopencm3

OBJS := main.o \
        host_service.o \
        clock.o \
        timebase.o \
        uart.o \
        ring_buffer.o \
        cobs.o \
        crc16.o \
        protocol.o

VPATH += app platform protocol

CFLAGS   += -std=c11 -Os -g3 -Wall -Wextra -Wshadow -Wundef -Wformat=2
CPPFLAGS += -MD -Iapp -Iplatform -Iprotocol
LDFLAGS  += -static -nostartfiles

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

LDLIBS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

LIBOPENCM3_F3 := $(OPENCM3_DIR)/lib/libopencm3_stm32f3.a
LIBDEPS += $(LIBOPENCM3_F3)

.PHONY: all clean libopencm3

all: $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex

libopencm3: $(LIBOPENCM3_F3)

$(LIBOPENCM3_F3):
	$(MAKE) -C $(OPENCM3_DIR)

clean:
	$(RM) -f $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex $(DEVICE).ld
	$(RM) -f $(OBJS) $(OBJS:.o=.d)

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk

-include $(OBJS:.o=.d)
