# clang cross compiles out of the box. the gcc cross compiler works too:
#   make CC=~/opt/cross/bin/i686-elf-gcc LD=~/opt/cross/bin/i686-elf-ld
CC := clang --target=i686-elf
LD := ld -m elf_i386
OBJCOPY := objcopy
HOST_CC := cc
QEMU := qemu-system-i386

# which bootloader the disk image boots with:
#   moos   - the moOS boot sector, fat16 without a partition table, kernel in
#            the reserved sectors
#   limine - limine (external/limine submodule) in the mbr, fat16 partition
#            with the kernel elf in /boot, booted through multiboot 1
BOOTLOADER ?= moos
ifeq ($(filter $(BOOTLOADER),moos limine),)
$(error BOOTLOADER must be moos or limine, not $(BOOTLOADER))
endif
IMAGE := ./bin/os-$(BOOTLOADER).img

# 16mb disk images
DISK_SECTORS := 32768

# moos layout: kernel is stored in the fat16 reserved sectors right after the
# boot sector. must stay in sync with KERNEL_SECTORS, ReservedSectors and
# NumSectors in bootloader.asm
KERNEL_SECTORS := 199
FAT_START_SECTOR := 200
SECTORS_PER_FAT := 32

# limine layout: the fat16 partition starts at 1 mb, limine's stage 2 goes in
# the gap after the mbr
LIMINE_DIR := ./external/limine
LIMINE_TOOL := ./build/limine/limine
PARTITION_START := 2048

# kernel.asm holds the entry stub, the linker script places it first
ASM_SRCS := src/bootloader/kernel.asm \
	src/kernel/idt/idt.asm \
	src/kernel/io/io.asm \
	src/kernel/memory/paging/page.asm \
	src/kernel/gdt/gdt.asm \
	src/kernel/task/tss.asm \
	src/kernel/task/task.asm

C_SRCS := src/kernel/kernel.c \
	src/drivers/screen/terminal.c \
	src/libc/string/string.c \
	src/kernel/idt/idt.c \
	src/kernel/memory/heap/heap.c \
	src/kernel/kmem/kheap.c \
	src/kernel/memory/paging/page.c \
	src/drivers/disk/disk.c \
	src/libc/stdlib/stdlib.c \
	src/libc/ctype/ctype.c \
	src/file_system/path_parser.c \
	src/drivers/disk/disk_stream.c \
	src/libc/stdio/stdio.c \
	src/file_system/file.c \
	src/file_system/fat/fat16.c \
	src/kernel/gdt/gdt.c \
	src/drivers/serial/serial.c \
	src/drivers/rtc/rtc.c \
	src/drivers/timer/pit.c \
	src/drivers/keyboard/keyboard.c \
	src/kernel/task/task.c \
	src/kernel/task/sched.c \
	src/kernel/task/elf.c \
	src/kernel/task/handle.c \
	src/kernel/task/pipe.c \
	src/kernel/syscall/syscall.c

ASM_OBJS := $(patsubst src/%.asm,build/%.asm.o,$(ASM_SRCS))
C_OBJS := $(patsubst src/%.c,build/%.o,$(C_SRCS))
FILES := $(ASM_OBJS) $(C_OBJS)

# user programs, elf executables copied to 0:/bin on the disk image
USER_PROGS := hello count crash spin rtdemo memtest cat wc sort cp echo ls grep \
	date time pipetest cowsay moose sh
USER_LIB_OBJS := build/user/lib/crt0.asm.o build/user/lib/syscall.asm.o \
	build/user/lib/moos.o
USER_BINS := $(patsubst %,build/user/%.elf,$(USER_PROGS))
# keep user objects, make would delete them as intermediate files
.SECONDARY: $(USER_BINS:.elf=.o) build/user/shline.o

FLAGS = -g -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fomit-frame-pointer -march=i686 \
	-mno-mmx -mno-sse -mno-sse2 -nostdlib -Wall -Werror -Wno-unused-function \
	-Wno-unused-label -Wno-unused-parameter -O0 -std=gnu99 -MMD -MP

.PHONY: all run test test-all clean

all: $(IMAGE)

# copies the test files and user programs into the fat16 file system at the
# mtools image spec $(1)
define populate_fs
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) ./test/fs_test.txt ::/
	MTOOLS_SKIP_CHECK=1 mmd -i $(1) ::/dir
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) ./build/long.txt ::/dir/
	MTOOLS_SKIP_CHECK=1 mmd -i $(1) ::/bin
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) $(USER_BINS) ::/bin/
endef

FS_FILES := ./test/fs_test.txt ./build/long.txt $(USER_BINS)

./bin/os-moos.img: ./bin/bootloader.bin ./bin/kernel.bin $(FS_FILES)
	rm -f $@
	dd if=/dev/zero of=$@ bs=512 count=$(DISK_SECTORS) status=none
	dd if=./bin/bootloader.bin of=$@ conv=notrunc status=none
	dd if=./bin/kernel.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	# first two fat entries hold the media type and end of chain marker
	printf '\370\377\377\377' | dd of=$@ bs=512 seek=$(FAT_START_SECTOR) conv=notrunc status=none
	printf '\370\377\377\377' | dd of=$@ bs=512 seek=$$(($(FAT_START_SECTOR) + $(SECTORS_PER_FAT))) conv=notrunc status=none
	$(call populate_fs,$@)

./bin/os-limine.img: ./bin/kernel.elf ./src/bootloader/limine.conf $(LIMINE_TOOL) $(FS_FILES)
	rm -f $@
	dd if=/dev/zero of=$@ bs=512 count=$(DISK_SECTORS) status=none
	# mbr partition table with one bootable fat16 partition
	echo 'start=$(PARTITION_START), type=06, bootable' | sfdisk --quiet $@
	mkfs.fat -F 16 -n MOOS --offset $(PARTITION_START) $@ \
		$$((($(DISK_SECTORS) - $(PARTITION_START)) / 2)) > /dev/null
	MTOOLS_SKIP_CHECK=1 mmd -i $@@@$(PARTITION_START)S ::/boot ::/boot/limine
	MTOOLS_SKIP_CHECK=1 mcopy -i $@@@$(PARTITION_START)S $(LIMINE_DIR)/limine-bios.sys ./src/bootloader/limine.conf ::/boot/limine/
	MTOOLS_SKIP_CHECK=1 mcopy -i $@@@$(PARTITION_START)S ./bin/kernel.elf ::/boot/moos.elf
	$(call populate_fs,$@@@$(PARTITION_START)S)
	$(LIMINE_TOOL) bios-install $@ > /dev/null

# limine's installer, built from the submodule into build/ so the submodule
# checkout stays clean
$(LIMINE_TOOL): $(LIMINE_DIR)/limine.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -g -O2 -pipe -std=c99 $< -o $@

$(LIMINE_DIR)/limine.c:
	@echo "limine submodule missing, run: git submodule update --init" && exit 1

build/user/%.elf: build/user/%.o $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ $< $(USER_LIB_OBJS)

# the shell is split into the command interpreter and the line editor
build/user/sh.elf: build/user/sh.o build/user/shline.o $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ build/user/sh.o build/user/shline.o $(USER_LIB_OBJS)

# multi cluster test file: 600 lines of "line NNNN\n"
./build/long.txt:
	@mkdir -p $(dir $@)
	seq -f 'line %04g' 1 600 > $@

# kernel elf, loaded by multiboot loaders and used for debug symbols
./bin/kernel.elf: $(FILES) ./src/linker.ld
	@mkdir -p $(dir $@)
	$(LD) -T ./src/linker.ld -o $@ $(FILES)

# flat image for the moOS bootloader, padded to the sectors it loads
./bin/kernel.bin: ./bin/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@size=$$(stat -c %s $@); max=$$(($(KERNEL_SECTORS) * 512)); \
	if [ $$size -gt $$max ]; then \
		echo "kernel.bin is $$size bytes, bootloader only loads $$max"; rm -f $@; exit 1; \
	fi
	truncate -s $$(($(KERNEL_SECTORS) * 512)) $@

./bin/bootloader.bin: ./src/bootloader/bootloader.asm
	@mkdir -p $(dir $@)
	nasm -f bin $< -o $@

build/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	nasm -f elf -g $< -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) -c $< -o $@

-include $(C_OBJS:.o=.d) $(USER_BINS:.elf=.d) build/user/lib/moos.d \
	build/user/shline.d

clean:
	rm -rf ./build/* ./bin/*

# kernel log also goes to the serial port, shown in qemu's serial0 view
run: all
	$(QEMU) -drive file=$(IMAGE),format=raw -rtc base=localtime

# boots headless, types commands into the shell through the qemu monitor and
# checks the kernel log written to the qemu debug console
test: all
	python3 ./scripts/qemu_test.py $(IMAGE)

# runs the tests with both bootloaders
test-all:
	$(MAKE) test BOOTLOADER=moos
	$(MAKE) test BOOTLOADER=limine
