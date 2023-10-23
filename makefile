CC := ~/opt/cross/bin/i686-elf-gcc
LD := ~/opt/cross/bin/i686-elf-ld


C_SOURCES = $(wildcard ./src/kernel/*.c ./src/drivers/*.c ./src/libc/string/*c)
HEADERS = $(wildcard ./src/kernel/*.h .src/drivers/*.h ./src/libc/string/*h)
OBJ = ${C_SOURCES:.c=.o}

os-image.bin: ./src/bootloader/bootloader.bin kernel.bin
	cat $^ > os-image.bin

kernel.bin: ./src/bootloader/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ --oformat binary

kernel.elf: ./src/bootloader/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ 

run: os-image.bin
	qemu-system-x86_64 -fda os-image.bin

%.o: %.c ${HEADERS}
	${CC}  -ffreestanding -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf ./bin/*.bin *.o *.bin *.elf
	rm -rf ./src/kernel/*.o ./src/bootloader/*.bin ./src/drivers/*.o ./src/bootloader/*.o
