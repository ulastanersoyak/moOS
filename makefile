# Compiler and linker settings
CC := ~/opt/cross/bin/i686-elf-gcc
LD := ~/opt/cross/bin/i686-elf-ld
NASM := nasm

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
DEBUG_DIR := debug

# Source files
BOOTLOADER_SRC := $(wildcard $(SRC_DIR)/bootloader/*.asm)
KERNEL_SRC := $(wildcard $(SRC_DIR)/kernel/*.c)
KERNEL_ENTRY_SRC := $(SRC_DIR)/bootloader/kernel_entry.asm

# Output files
BOOTLOADER_BIN := $(BIN_DIR)/bootloader.bin
KERNEL_BIN := $(BIN_DIR)/kernel.bin
OS_IMAGE_BIN := $(BIN_DIR)/os_image.bin
KERNEL_ENTRY_OBJ := $(BUILD_DIR)/kernel_entry.o
KERNEL_OBJ := $(BUILD_DIR)/kernel.o
KERNEL_DIS := $(DEBUG_DIR)/kernel.dis

# Flags
CFLAGS := -ffreestanding
LDFLAGS := -Ttext 0x1000 --oformat binary

# Targets
all: $(OS_IMAGE_BIN) debug

$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ)
	$(LD) -o $@ $(LDFLAGS) $^

$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC)
	$(NASM) $< -f elf -o $@

$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(BOOTLOADER_BIN): $(BOOTLOADER_SRC)
	$(NASM) $< -f bin -o $@

$(OS_IMAGE_BIN): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	cat $^ > $@

qemu: $(OS_IMAGE_BIN)
	qemu-system-x86_64 -fda $<

debug: $(KERNEL_BIN)
	ndisasm -b 32 $< > $(KERNEL_DIS)

clear:
	rm -f $(BIN_DIR)/*.bin $(DEBUG_DIR)/*.dis $(BUILD_DIR)/*.o

.PHONY: all qemu debug clear

