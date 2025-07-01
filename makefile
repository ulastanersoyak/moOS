# clang cross compiles out of the box. the gcc cross compiler works too:
#   make CC=~/opt/cross/bin/i686-elf-gcc LD=~/opt/cross/bin/i686-elf-ld
CC := clang --target=i686-elf
LD := ld -m elf_i386
QEMU := qemu-system-i386

# kernel is stored in the fat16 reserved sectors right after the boot sector.
# must stay in sync with KERNEL_SECTORS in bootloader.asm and ReservedSectors
KERNEL_SECTORS := 199
# 16mb disk image, must match NumSectors in bootloader.asm
DISK_SECTORS := 32768
FAT_START_SECTOR := 200
SECTORS_PER_FAT := 32

# kernel.asm must be first so _start ends up at 0x100000
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
	src/kernel/ascii/ascii.c \
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
	src/kernel/syscall/syscall.c \
	src/kernel/shell/shell.c \
	src/kernel/shell/readline.c

ASM_OBJS := $(patsubst src/%.asm,build/%.asm.o,$(ASM_SRCS))
C_OBJS := $(patsubst src/%.c,build/%.o,$(C_SRCS))
FILES := $(ASM_OBJS) $(C_OBJS)

FLAGS = -g -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fomit-frame-pointer -march=i686 \
	-mno-mmx -mno-sse -mno-sse2 -nostdlib -Wall -Werror -Wno-unused-function \
	-Wno-unused-label -Wno-unused-parameter -O0 -std=gnu99 -MMD -MP

.PHONY: all run test clean

all: ./bin/os.bin

./bin/os.bin: ./bin/bootloader.bin ./bin/kernel.bin ./test/fs_test.txt ./build/long.txt
	rm -f $@
	dd if=/dev/zero of=$@ bs=512 count=$(DISK_SECTORS) status=none
	dd if=./bin/bootloader.bin of=$@ conv=notrunc status=none
	dd if=./bin/kernel.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	# first two fat entries hold the media type and end of chain marker
	printf '\370\377\377\377' | dd of=$@ bs=512 seek=$(FAT_START_SECTOR) conv=notrunc status=none
	printf '\370\377\377\377' | dd of=$@ bs=512 seek=$$(($(FAT_START_SECTOR) + $(SECTORS_PER_FAT))) conv=notrunc status=none
	MTOOLS_SKIP_CHECK=1 mcopy -i $@ ./test/fs_test.txt ::/
	MTOOLS_SKIP_CHECK=1 mmd -i $@ ::/dir
	MTOOLS_SKIP_CHECK=1 mcopy -i $@ ./build/long.txt ::/dir/

# multi cluster test file: 600 lines of "line NNNN\n"
./build/long.txt:
	@mkdir -p $(dir $@)
	seq -f 'line %04g' 1 600 > $@

./bin/kernel.bin: $(FILES) ./src/linker.ld
	@mkdir -p $(dir $@)
	$(LD) -g -relocatable $(FILES) -o ./build/kernelfull.o
	$(LD) -T ./src/linker.ld -o $@ ./build/kernelfull.o
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

-include $(C_OBJS:.o=.d)

clean:
	rm -rf ./build/* ./bin/*

# kernel log also goes to the serial port, shown in qemu's serial0 view
run: all
	$(QEMU) -drive file=./bin/os.bin,format=raw

# boots headless for a few seconds and checks the kernel log written to the
# qemu debug console
test: all
	-timeout 5 $(QEMU) -drive file=./bin/os.bin,format=raw -display none \
		-debugcon file:./build/debugcon.log
	@cat ./build/debugcon.log
	@! grep -q 'ER\]\|panic\|exception' ./build/debugcon.log
	@grep -q 'enable interrupts' ./build/debugcon.log
	@echo "boot test passed"
