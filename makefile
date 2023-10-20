cross_compiler := ~/opt/cross/bin/i686-elf-gcc
linker := ~/opt/cross/bin/i686-elf-ld 

all: ./bin/os_image.bin debug

./bin/kernel.bin: ./build/kernel_entry.o ./build/kernel.o
	$(linker) -o $@ -Ttext 0x1000 $^ --oformat binary

./build/kernel_entry.o: ./src/kernel/kernel_entry.asm
	nasm $< -f elf -o $@

./build/kernel.o: ./src/kernel/kernel.c
	$(cross_compiler) -ffreestanding -c $< -o $@

./bin/bootloader.bin: ./src/bootloader/bootloader.asm
	nasm $< -f bin -o $@

./bin/os_image.bin: ./bin/bootloader.bin ./bin/kernel.bin
	cat $^ > $@

qemu: ./bin/os_image.bin
	qemu-system-x86_64 -fda $<

debug: ./bin/kernel.bin
	ndisasm -b 32 $< > ./debug/kernel.dis

clear:
	rm ./bin/*.bin ./debug/*.dis ./build/*.o
