# CH32V003 Firmware Makefile
# Cross-compiles C / Assembly code for QingKe RISC-V V2A core

CC ?= riscv64-unknown-elf-gcc
OBJCOPY ?= riscv64-unknown-elf-objcopy
SIZE ?= riscv64-unknown-elf-size

ARCH = rv32ec
ABI = ilp32e

CFLAGS = -march=$(ARCH) -mabi=$(ABI) -Os -nostdlib -Wall
LDFLAGS = -T ch32v003.ld

SRCS = startup.s main.c
TARGET = blink

all: $(TARGET).bin $(TARGET).elf
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(SRCS) ch32v003.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f $(TARGET).elf $(TARGET).bin

.PHONY: all clean
