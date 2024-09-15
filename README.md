# moos

a 32-bit operating system written in c and assembly. it implements basic os features like memory management, device drivers, and system initialization.

![Screenshot from 2023-11-05 17-51-21](https://github.com/ulastanersoyak/moOS/assets/92662728/c7377806-eaa6-427b-b570-86b314c03937)

## features

currently working:
- vga display driver
- basic interrupt handling
- memory management with paging
- heap memory allocation
- disk i/o operations

in development:
- full interrupt system
- keyboard support
- file system
- user space programs

## getting started

requirements:
- i686-gcc cross-compiler
- nasm
- qemu
- make

to run:
```bash
make run
```

## project structure
- `src/bootloader/` - system boot process
- `src/kernel/` - core os functionality
- `src/drivers/` - hardware drivers
- `src/libc/` - standard library implementation

## installation

### arch linux
```bash
sudo pacman -S git qemu make nasm
```

for i686-gcc cross-compiler, follow [this guide](https://wiki.osdev.org/GCC_Cross-Compiler)

## license
see license file for details
