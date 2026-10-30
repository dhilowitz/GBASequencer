# Builds gbasequencer.gba with any arm-none-eabi GCC (no devkitPro needed).
#   make          build the ROM
#   make assets   regenerate src/gen from gfx/ and samples/ (needs Python with Pillow and numpy)
#   make test     run the headless tests (needs tools/harness built: see README)

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
PYTHON ?= python3

ROM   = gbasequencer.gba
BUILD = build
SRCS  = $(wildcard src/*.c) $(wildcard src/gen/*.c)
OBJS  = $(patsubst %.c,$(BUILD)/%.o,$(SRCS)) $(BUILD)/src/crt0.o $(BUILD)/src/irq.o

ARCH   = -mcpu=arm7tdmi -mthumb -mthumb-interwork -mlong-calls
CFLAGS = $(ARCH) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -ffreestanding -fno-strict-aliasing \
         -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS = $(ARCH) -nostartfiles -nostdlib -T src/gba.ld -Wl,--gc-sections -Wl,-Map,$(BUILD)/gbasequencer.map

all: $(ROM)

$(ROM): $(BUILD)/gbasequencer.elf
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/gbafix.py $@

$(BUILD)/gbasequencer.elf: $(OBJS) src/gba.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/src/%.o: src/%.s
	@mkdir -p $(dir $@)
	$(CC) $(ARCH) -marm -c -o $@ $<

assets:
	$(PYTHON) tools/mkassets.py

test: $(ROM)
	$(PYTHON) tests/check.py

clean:
	rm -rf $(BUILD) $(ROM)

-include $(OBJS:.o=.d)
.PHONY: all assets test clean
