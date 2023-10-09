cross_compiler := ~/opt/cross/bin/i686-elf-gcc
linker := ~/opt/cross/bin/i686-elf-ld 

all: qemu

./bin/kernel.bin: ./build/kernel_entry.o ./build/kernel.o
	$(linker) -o $@ -Ttext 0x1000 $^ --oformat binary

./build/kernel_entry.o: ./src/kernel/kernel_entry.asm
	nasm $< -f elf -o $@

./build/kernel.o: ./src/kernel/kernel.c
	$(cross_compiler) -ffreestanding -c $< -o $@

./debug/kernel.dis: ./bin/kernel.bin
	ndisasm -b 32 $< > ./debug/kernel.dis

./bin/bootloader.bin: ./src/bootloader/bootloader.asm
	nasm $< -f bin -o $@

./bin/os_image.bin: ./bin/bootloader.bin ./bin/kernel.bin
	cat $^ > $@

qemu: ./bin/os_image.bin
	qemu-system-x86_64 -fda $<

clear:
	rm ./bin/*.bin ./debug/*.dis ./build/*.o
