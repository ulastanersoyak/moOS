CC := ~/opt/cross/bin/i686-elf-gcc
LD := ~/opt/cross/bin/i686-elf-ld

FILES = ./build/kernel.asm.o ./build/kernel.o ./build/terminal.o ./build/libc.o ./build/idt.asm.o ./build/idt.o ./build/io.asm.o
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

.PHONY: build_dirs

all: build_dirs ./bin/bootloader.bin ./bin/kernel.bin 
	rm -rf ./bin/os.bin
	dd if=./bin/bootloader.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin

./bin/kernel.bin: $(FILES)
	$(LD) -g -relocatable $(FILES) -o ./build/kernelfull.o
	$(CC) $(FLAGS) -T ./linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o 

./bin/bootloader.bin: ./src/bootloader/bootloader.asm
	nasm -f bin ./src/bootloader/bootloader.asm -o ./bin/bootloader.bin

./build/kernel.asm.o: ./src/bootloader/kernel.asm
	nasm -f elf -g ./src/bootloader/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel/kernel.c
	$(CC) -I./src/kernel $(FLAGS) -std=gnu99 -c ./src/kernel/kernel.c -o ./build/kernel.o

./build/terminal.o : ./src/drivers/terminal.c
	$(CC) -I./src/drivers/ $(FLAGS) -std=gnu99 -c ./src/drivers/terminal.c -o ./build/terminal.o

./build/libc.o: ./src/libc/string/string.c
	$(CC) -I./src/libc/string/ $(FLAGS) -std=gnu99 -c ./src/libc/string/string.c -o ./build/libc.o

./build/idt.asm.o: ./src/kernel/idt/idt.asm
	nasm -f elf -g ./src/kernel/idt/idt.asm -o ./build/idt.asm.o

./build/idt.o: ./src/kernel/idt/idt.c
	$(CC) -I./src/kernel/idt/ $(FLAGS) -std=gnu99 -c ./src/kernel/idt/idt.c -o ./build/idt.o

./build/io.asm.o : ./src/kernel/io/io.asm
	nasm -f elf -g ./src/kernel/io/io.asm -o ./build/io.asm.o


build_dirs:
	if [ ! -d "bin" ]; then mkdir bin; fi
	if [ ! -d "build" ]; then mkdir build; fi

clean:
	rm -rf ./build/* ./bin/*

run: all 
	qemu-system-x86_64 -hda ./bin/os.bin

