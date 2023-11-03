CC := ~/opt/cross/bin/i686-elf-gcc
LD := ~/opt/cross/bin/i686-elf-ld

FILES = ./build/kernel.asm.o ./build/kernel.o ./build/terminal.o ./build/libc.o ./build/idt.asm.o ./build/idt.o ./build/io.asm.o ./build/heap.o ./build/kheap.o ./build/ascii.o ./build/page.asm.o ./build/page.o ./build/disk.o ./build/stdlib.o ./build/ctype.o ./build/path_parser.o ./build/disk_stream.o ./build/stdio.o ./build/file_system.o
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

.PHONY: build_dirs

all: build_dirs ./bin/bootloader.bin ./bin/kernel.bin 
	rm -rf ./bin/os.bin
	dd if=./bin/bootloader.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./file_sys_test.txt /mnt/d
	sudo umount /mnt/d

./bin/kernel.bin: $(FILES)
	$(LD) -g -relocatable $(FILES) -o ./build/kernelfull.o
	$(CC) $(FLAGS) -T ./linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o 

./bin/bootloader.bin: ./src/bootloader/bootloader.asm
	nasm -f bin ./src/bootloader/bootloader.asm -o ./bin/bootloader.bin

./build/kernel.asm.o: ./src/bootloader/kernel.asm
	nasm -f elf -g ./src/bootloader/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel/kernel.c
	$(CC) -I./src/kernel $(FLAGS) -std=gnu99 -c ./src/kernel/kernel.c -o ./build/kernel.o

./build/terminal.o : ./src/drivers/screen/terminal.c
	$(CC) -I./src/drivers/ $(FLAGS) -std=gnu99 -c ./src/drivers/screen/terminal.c -o ./build/terminal.o

./build/libc.o: ./src/libc/string/string.c
	$(CC) -I./src/libc/string/ $(FLAGS) -std=gnu99 -c ./src/libc/string/string.c -o ./build/libc.o

./build/idt.asm.o: ./src/kernel/idt/idt.asm
	nasm -f elf -g ./src/kernel/idt/idt.asm -o ./build/idt.asm.o

./build/idt.o: ./src/kernel/idt/idt.c
	$(CC) -I./src/kernel/idt/ $(FLAGS) -std=gnu99 -c ./src/kernel/idt/idt.c -o ./build/idt.o

./build/io.asm.o : ./src/kernel/io/io.asm
	nasm -f elf -g ./src/kernel/io/io.asm -o ./build/io.asm.o

./build/heap.o : ./src/memory/heap/heap.c
	$(CC) -I./src/memory/heap/ $(FLAGS) -std=gnu99 -c ./src/memory/heap/heap.c -o ./build/heap.o

./build/kheap.o : ./src/kernel/kmem/kheap.c
	$(CC) -I./src/kernel/kmem/ $(FLAGS) -std=gnu99 -c ./src/kernel/kmem/kheap.c -o ./build/kheap.o

./build/ascii.o : ./src/kernel/ascii/ascii.c
	$(CC) -I./src/kernel/ascii/ $(FLAGS) -std=gnu99 -c ./src/kernel/ascii/ascii.c -o ./build/ascii.o

./build/page.asm.o : ./src/memory/paging/page.asm
	nasm -f elf -g ./src/memory/paging/page.asm -o ./build/page.asm.o

./build/page.o : ./src/memory/paging/page.c
	$(CC) -I./src/memory/paging/ $(FLAGS) -std=gnu99 -c ./src/memory/paging/page.c -o ./build/page.o

./build/disk.o : ./src/drivers/disk/disk.c
	$(CC) -I./src/drivers/disk/ $(FLAGS) -std=gnu99 -c ./src/drivers/disk/disk.c -o ./build/disk.o

./build/stdlib.o : ./src/libc/stdlib/stdlib.c
	$(CC) -I./src/libc/stdlib/ $(FLAGS) -std=gnu99 -c ./src/libc/stdlib/stdlib.c -o ./build/stdlib.o

./build/ctype.o : ./src/libc/ctype/ctype.c
	$(CC) -I./src/libc/ctype/ $(FLAGS) -std=gnu99 -c ./src/libc/ctype/ctype.c -o ./build/ctype.o

./build/path_parser.o : ./src/file_system/path_parser.c
	$(CC) -I./src/file_system/ $(FLAGS) ./src/file_system/path_parser.c -std=gnu99 -c -o ./build/path_parser.o
	
./build/disk_stream.o : ./src/drivers/disk/disk_stream.c
	$(CC) -I./src/drivers/disk/ $(FLAGS) ./src/drivers/disk/disk_stream.c -std=gnu99 -c -o ./build/disk_stream.o

./build/stdio.o : ./src/libc/stdio/stdio.c
	$(CC) -I./src/libc/stdio/ $(FLAGS) -std=gnu99 -c ./src/libc/stdio/stdio.c -o ./build/stdio.o
	
./build/file_system.o : ./src/file_system/file.c
	$(CC) -I./src/file_system/ $(FLAGS) ./src/file_system/file.c -std=gnu99 -c -o ./build/file_system.o

build_dirs:
	if [ ! -d "bin" ]; then mkdir bin; fi
	if [ ! -d "build" ]; then mkdir build; fi

clean:
	rm -rf ./build/* ./bin/*

run: all 
	qemu-system-x86_64 -hda ./bin/os.bin

