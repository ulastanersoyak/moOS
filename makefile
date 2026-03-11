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
	src/kernel/task/task.asm \
	src/kernel/fpu/fpu.asm

C_SRCS := src/kernel/kernel.c \
	src/drivers/screen/terminal.c \
	src/drivers/screen/vga_gfx.c \
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
	src/kernel/task/mutex.c \
	src/kernel/fpu/fpu.c \
	src/kernel/syscall/syscall.c

ASM_OBJS := $(patsubst src/%.asm,build/%.asm.o,$(ASM_SRCS))
C_OBJS := $(patsubst src/%.c,build/%.o,$(C_SRCS))
FILES := $(ASM_OBJS) $(C_OBJS)

# user programs, elf executables copied to 0:/bin on the disk image
USER_PROGS := hello count crash spin rtdemo memtest cat wc sort cp echo ls grep \
	date time pipetest cowsay moose sh pidemo edfdemo fputest gfxdemo keys \
	libctest doom
USER_LIB_OBJS := build/user/lib/crt0.asm.o build/user/lib/syscall.asm.o \
	build/user/lib/moos.o build/user/lib/int64.o
USER_BINS := $(patsubst %,build/user/%.elf,$(USER_PROGS))

# a standard c library on top of the user library for programs written
# against the c headers (stdio.h, string.h, ...) instead of moos.h
LIBC_OBJS := $(patsubst src/%.c,build/%.o,$(wildcard src/user/libc/*.c))
# -nostdinc keeps the host's headers out, the compiler's own freestanding
# ones (stdint.h, stdarg.h, limits.h, ...) stay
LIBC_INCLUDE := -nostdinc -isystem $(shell $(CC) -print-resource-dir)/include \
	-I./src/user/libc/include
LIBC_PROGS := libctest

# doom, the doomgeneric submodule built against the c library. it renders
# 320x200 with a 256 colour palette, vga mode 13h. its own code is built
# with optimizations and without warnings, src/user/doom.c connects it to
# moos
DOOM_DIR := ./external/doomgeneric/doomgeneric
DOOM_SRCS := dummy am_map doomdef doomstat dstrings d_event d_items d_iwad \
	d_loop d_main d_mode d_net f_finale f_wipe g_game hu_lib hu_stuff info \
	i_cdmus i_endoom i_joystick i_scale i_sound i_system i_timer memio \
	m_argv m_bbox m_cheat m_config m_controls m_fixed m_menu m_misc \
	m_random p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map \
	p_maputl p_mobj p_plats p_pspr p_saveg p_setup p_sight p_spec \
	p_switch p_telept p_tick p_user r_bsp r_data r_draw r_main r_plane \
	r_segs r_sky r_things sha1 sounds statdump st_lib st_stuff s_sound \
	tables v_video wi_stuff w_checksum w_file w_main w_wad z_zone \
	w_file_stdc i_input i_video doomgeneric
DOOM_OBJS := $(patsubst %,build/doom/%.o,$(DOOM_SRCS))
DOOM_DEFINES := -DCMAP256 -DDOOMGENERIC_RESX=320 -DDOOMGENERIC_RESY=200
DOOM_FLAGS = -O2 -g -ffreestanding -fno-builtin -fno-pic -fno-pie \
	-fno-stack-protector -fno-asynchronous-unwind-tables -march=i686 \
	-mno-mmx -mno-sse -mno-sse2 -nostdlib -std=gnu99 -w -MMD -MP \
	$(LIBC_INCLUDE) $(DOOM_DEFINES)
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

build/user/libc/%.o: FLAGS += $(LIBC_INCLUDE)
$(patsubst %,build/user/%.o,$(LIBC_PROGS)): FLAGS += $(LIBC_INCLUDE)
$(patsubst %,build/user/%.elf,$(LIBC_PROGS)): build/user/%.elf: build/user/%.o $(LIBC_OBJS) $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ $< $(LIBC_OBJS) $(USER_LIB_OBJS)

build/doom/%.o: $(DOOM_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(DOOM_FLAGS) -c $< -o $@

build/user/doom.o: FLAGS += $(LIBC_INCLUDE) -I$(DOOM_DIR) $(DOOM_DEFINES)
build/user/doom.elf: build/user/doom.o $(DOOM_OBJS) $(LIBC_OBJS) $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ $< $(DOOM_OBJS) $(LIBC_OBJS) $(USER_LIB_OBJS)

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
	build/user/lib/int64.d $(LIBC_OBJS:.o=.d) $(DOOM_OBJS:.o=.d) \
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
