all: ./src/bootloader/bootloader.asm
	nasm -fbin ./src/bootloader/bootloader.asm -o ./bin/bootloader.bin

qemu:
	qemu-system-x86_64 ./bin/bootloader.bin 

clear:
	rm ./bin/bootloader.bin
