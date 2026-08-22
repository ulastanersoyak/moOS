# clang cross compiles out of the box. the gcc cross compiler works too:
#   make CC=~/opt/cross/bin/i686-elf-gcc LD=~/opt/cross/bin/i686-elf-ld
CC := clang --target=i686-elf
LD := ld -m elf_i386
OBJCOPY := objcopy
AR := ar
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
	src/drivers/mouse/mouse.c \
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
	libctest doom desk tcc galaga
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
# the shareware wad is free to give away but not ours to commit. it comes
# from debian's doom-wad-shareware package and must be doom1.wad 1.9
DOOM_WAD := ./build/wad/doom1.wad
DOOM_WAD_DEB := https://deb.debian.org/debian/pool/non-free/d/doom-wad-shareware/doom-wad-shareware_1.9.fixed-5_all.deb
DOOM_WAD_SHA1 := 5b2e249b9c5133ec987b3ea77596381dc0d6bc1d
DOOM_DEFINES := -DCMAP256 -DDOOMGENERIC_RESX=320 -DDOOMGENERIC_RESY=200
DOOM_FLAGS = -O2 -g -ffreestanding -fno-builtin -fno-pic -fno-pie \
	-fno-stack-protector -fno-asynchronous-unwind-tables -march=i686 \
	-mno-mmx -mno-sse -mno-sse2 -nostdlib -std=gnu99 -w -MMD -MP \
	$(LIBC_INCLUDE) $(DOOM_DEFINES)
# galaga, the xgalaga-sdl submodule (a galaga clone) built against the c
# library. src/user/galaga.c has the part of sdl 1.2 it uses and
# src/user/galaga/ the sdl headers, a png loader and its config.h. its
# images and levels go to 0:/galaga
GALAGA_DIR := ./external/xgalaga-sdl
GALAGA_SRCS := SFont TFont data explosions frate highscore images joystick \
	main pathfile paths prize score sdl sound title
GALAGA_OBJS := $(patsubst %,build/galaga/%.o,$(GALAGA_SRCS)) build/galaga/png.o
GALAGA_INCLUDE := $(LIBC_INCLUDE) -I./src/user/galaga
GALAGA_FLAGS = -O2 -g -ffreestanding -fno-builtin -fno-pic -fno-pie \
	-fno-stack-protector -fno-asynchronous-unwind-tables -march=i686 \
	-mno-mmx -mno-sse -mno-sse2 -nostdlib -std=gnu99 -w -MMD -MP \
	$(GALAGA_INCLUDE) -include ./src/user/galaga/config.h
GALAGA_DATA := $(wildcard $(GALAGA_DIR)/images/*.png) \
	$(wildcard $(GALAGA_DIR)/levels/*.xgl) $(GALAGA_DIR)/CREDITS
# tcc, the tiny c compiler from the tinycc submodule. it runs on moos and
# builds static programs against the c library, which goes onto the disk as
# /lib/libc.a with its headers in /include. tcc's own headers and runtime
# (libtcc1.a) go to /lib/tcc. the same compiler built for the host,
# i386-moos-tcc, builds that runtime and can build moos programs too
TCC_DIR := ./external/tinycc
TCC_FLAGS = -O2 -g -ffreestanding -fno-builtin -fno-pic -fno-pie \
	-fno-stack-protector -fno-asynchronous-unwind-tables -march=i686 \
	-mno-mmx -mno-sse -mno-sse2 -nostdlib -std=gnu99 -w -MMD -MP \
	$(LIBC_INCLUDE) -I./src/user/tcc -I./build/tcc -I$(TCC_DIR)
TCC_HOST := ./build/tcc/i386-moos-tcc
SYSROOT := ./build/sysroot
TCC_HEADERS := $(filter-out %/tccdefs.h,$(wildcard $(TCC_DIR)/include/*.h)) \
	$(wildcard src/user/tcc/include/*.h)
LIBC_HEADERS := $(shell find src/user/libc/include -name '*.h') src/user/lib/moos.h
LIBTCC1_SRCS := libtcc1.c alloca.S alloca-bt.S builtin.c stdatomic.c atomic.S
LIBTCC1_OBJS := $(patsubst %,build/tcc/lib/%.o,$(LIBTCC1_SRCS))
# 64 bit division comes from libtcc1.a, int64.o would define it twice
SYSROOT_LIBC_OBJS := $(LIBC_OBJS) build/user/lib/moos.o build/user/lib/syscall.asm.o

# keep user objects, make would delete them as intermediate files
.SECONDARY: $(USER_BINS:.elf=.o) build/user/shline.o build/user/deskdraw.o \
	build/user/deskedit.o build/user/deskterm.o

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
	MTOOLS_SKIP_CHECK=1 mmd -i $(1) ::/doom
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) $(DOOM_WAD) ::/doom/
	MTOOLS_SKIP_CHECK=1 mmd -i $(1) ::/galaga ::/galaga/images ::/galaga/levels
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) $(GALAGA_DIR)/images/*.png ::/galaga/images/
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) $(GALAGA_DIR)/levels/*.xgl ::/galaga/levels/
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) $(GALAGA_DIR)/CREDITS ::/galaga/
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) -s $(SYSROOT)/lib $(SYSROOT)/include ::/
	MTOOLS_SKIP_CHECK=1 mmd -i $(1) ::/src
	MTOOLS_SKIP_CHECK=1 mcopy -i $(1) ./test/hello.c ./src/user/cowsay.c ::/src/
endef

FS_FILES := ./test/fs_test.txt ./build/long.txt $(USER_BINS) $(DOOM_WAD) \
	$(GALAGA_DATA) \
	./build/sysroot.stamp ./test/hello.c ./src/user/cowsay.c

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

build/galaga/%.o: $(GALAGA_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(GALAGA_FLAGS) -c $< -o $@

build/galaga/png.o: src/user/galaga/png.c
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(GALAGA_INCLUDE) -c $< -o $@

$(GALAGA_DIR)/src/main.c:
	@echo "xgalaga-sdl submodule missing, run: git submodule update --init" && exit 1

# SDL_Flip shrinks every frame, worth optimizing
build/user/galaga.o: FLAGS += $(GALAGA_INCLUDE) -O2
build/user/galaga.elf: build/user/galaga.o $(GALAGA_OBJS) $(LIBC_OBJS) $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ $< $(GALAGA_OBJS) $(LIBC_OBJS) $(USER_LIB_OBJS)

# tccdefs.h holds tcc's predefined macros, it is built into tcc as strings
./build/tcc/tccdefs_.h: $(TCC_DIR)/include/tccdefs.h $(TCC_DIR)/conftest.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -DC2STR $(TCC_DIR)/conftest.c -o ./build/tcc/c2str
	./build/tcc/c2str $< $@

$(TCC_DIR)/tcc.c:
	@echo "tinycc submodule missing, run: git submodule update --init" && exit 1

build/user/tcc.o: src/user/tcc/onesource.c ./build/tcc/tccdefs_.h $(TCC_DIR)/tcc.c
	@mkdir -p $(dir $@)
	$(CC) $(TCC_FLAGS) -c $< -o $@

build/user/tcc.elf: build/user/tcc.o $(LIBC_OBJS) $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ $< $(LIBC_OBJS) $(USER_LIB_OBJS)

# the cross compiler looks for headers and libraries in build/sysroot
$(TCC_HOST): src/user/tcc/onesource.c src/user/tcc/config.h ./build/tcc/tccdefs_.h $(TCC_DIR)/tcc.c
	$(HOST_CC) -O2 -g -w -DCONFIG_SYSROOT=\"$(abspath $(SYSROOT))\" \
		-I./src/user/tcc -I./build/tcc -I$(TCC_DIR) $< -o $@ -lm

build/tcc/lib/%.o: $(TCC_DIR)/lib/% $(TCC_HOST)
	@mkdir -p $(dir $@)
	$(TCC_HOST) -c -I$(TCC_DIR) $< -o $@

# what tcc needs on the disk. crti.o and crtn.o are empty, moos has no
# constructors. moos.h goes to /include/lib, programs include "lib/moos.h"
# like in src/user
./build/sysroot.stamp: $(TCC_HEADERS) $(LIBC_HEADERS) $(SYSROOT_LIBC_OBJS) \
		build/user/lib/crt0.asm.o $(LIBTCC1_OBJS)
	rm -rf $(SYSROOT)
	mkdir -p $(SYSROOT)/lib/tcc/include $(SYSROOT)/include/lib
	cp $(TCC_HEADERS) $(SYSROOT)/lib/tcc/include/
	cp -r src/user/libc/include/. $(SYSROOT)/include/
	cp src/user/lib/moos.h $(SYSROOT)/include/lib/
	$(AR) rcs $(SYSROOT)/lib/libc.a $(SYSROOT_LIBC_OBJS)
	cp build/user/lib/crt0.asm.o $(SYSROOT)/lib/crt1.o
	$(CC) -c -x c /dev/null -o $(SYSROOT)/lib/crti.o
	cp $(SYSROOT)/lib/crti.o $(SYSROOT)/lib/crtn.o
	$(TCC_HOST) -ar rcs $(SYSROOT)/lib/tcc/libtcc1.a $(LIBTCC1_OBJS)
	touch $@

# the shell is split into the command interpreter and the line editor
build/user/sh.elf: build/user/sh.o build/user/shline.o $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ build/user/sh.o build/user/shline.o $(USER_LIB_OBJS)

# the desktop is split into windows and menus, the drawing, the editor and
# the terminal
DESK_OBJS := build/user/desk.o build/user/deskdraw.o build/user/deskedit.o \
	build/user/deskterm.o
build/user/desk.elf: $(DESK_OBJS) $(USER_LIB_OBJS) ./src/user/linker.ld
	$(LD) -s -z noexecstack -T ./src/user/linker.ld -o $@ $(DESK_OBJS) $(USER_LIB_OBJS)

$(DOOM_WAD):
	@mkdir -p $(dir $@)deb
	curl -fsSL -o $(dir $@)deb/shareware.deb $(DOOM_WAD_DEB)
	cd $(dir $@)deb && ar x shareware.deb && tar -xf data.tar.xz ./usr/share/games/doom/doom1.wad
	echo "$(DOOM_WAD_SHA1)  $(dir $@)deb/usr/share/games/doom/doom1.wad" | sha1sum -c --quiet
	mv $(dir $@)deb/usr/share/games/doom/doom1.wad $@
	rm -rf $(dir $@)deb

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
	$(GALAGA_OBJS:.o=.d) \
	build/user/shline.d build/user/deskdraw.d build/user/deskedit.d \
	build/user/deskterm.d

# keeps the downloaded wad
clean:
	find ./build -mindepth 1 -maxdepth 1 ! -name wad -exec rm -rf {} +
	rm -rf ./bin/*

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
