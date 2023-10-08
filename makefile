all: ./src/bootloader/main.asm
	nasm -fbin ./src/bootloader/main.asm -o ./bin/boot.bin

qemu:
	qemu-system-x86_64 ./bin/boot.bin 

clear:
	rm ./bin/boot.bin
