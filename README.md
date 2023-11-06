## Table of Contents
1. [Introduction](#introduction)
2. [Project Structure](#project-structure)
3. [Installation](#installation)
    - [Linux](#linux)
    - [Mac](#mac)
    - [Windows](#windows)
4. [Contribution](#contribution)

<a name="introduction"></a>
## Introduction
moOS is a 32-bit operating system project in its early stages, featuring a dedicated bootloader, kernel, and standard library. Presently, the system is at a foundational stage, not yet offering specific functionalities, but the groundwork has been laid down.

My future objectives for moOS encompass a multi-threaded kernel, the potential development of its file system (such as considering FAT16), the creation of an independent shell, and the aspiration to run the original Doom game. Additionally, the project aims to incorporate its own text editor.


![Screenshot from 2023-11-05 17-51-21](https://github.com/ulastanersoyak/moOS/assets/92662728/c7377806-eaa6-427b-b570-86b314c03937)
## Project Structure:
```bash
moOS/
├── bin
├── build
├── file_sys_test.txt
├── find_todos.sh
├── LICENSE
├── linker.ld
├── makefile
├── moosv6.png
├── README.md
├── run.sh
└── src
    ├── bootloader
    │   ├── bootloader.asm
    │   └── kernel.asm
    ├── drivers
    │   ├── disk
    │   │   ├── disk.c
    │   │   ├── disk.h
    │   │   ├── disk_stream.c
    │   │   └── disk_stream.h
    │   └── screen
    │       ├── terminal.c
    │       ├── terminal.h
    │       └── vga.h
    ├── file_system
    │   ├── fat
    │   │   ├── fat16.c
    │   │   └── fat16.h
    │   ├── file.c
    │   ├── file.h
    │   ├── path_parser.c
    │   └── path_parser.h
    ├── kernel
    │   ├── ascii
    │   │   ├── ascii.c
    │   │   └── ascii.h
    │   ├── config.h
    │   ├── idt
    │   │   ├── idt.asm
    │   │   ├── idt.c
    │   │   └── idt.h
    │   ├── io
    │   │   ├── io.asm
    │   │   └── io.h
    │   ├── kernel.c
    │   ├── kernel.h
    │   ├── kmem
    │   │   ├── kheap.c
    │   │   └── kheap.h
    │   └── memory
    │       ├── heap
    │       │   ├── heap.c
    │       │   └── heap.h
    │       └── paging
    │           ├── page.asm
    │           ├── page.c
    │           └── page.h
    └── libc
        ├── ctype
        │   ├── ctype.c
        │   └── ctype.h
        ├── stdio
        │   ├── stdio.c
        │   └── stdio.h
        ├── stdlib
        │   ├── stdlib.c
        │   └── stdlib.h
        └── string
            ├── string.c
            └── string.h

```

## Installation:

### Linux:
To work with moOS on Linux, ensure you have the following dependencies installed:
- Git: `sudo apt-get install git` (for Ubuntu/Debian)
- QEMU: `sudo apt-get install qemu-system-x86`
- Make: `sudo apt-get install make`
- NASM: `sudo apt-get install nasm`
- i686-gcc cross-compiler and linker: Follow [this guide](https://wiki.osdev.org/GCC_Cross-Compiler).

Once dependencies are set, clone the repository and compile the code:
```bash
git clone git@github.com:ulastanersoyak/moOS.git
cd ./moOS
make # To compile the code
make run # To run moOS in QEMU
./run.sh  # To clean directories, recompile, and run moOS in QEMU
```

### Mac:

Steps above will **probably** work fine.

### Windows:

I have no idea how you can run it on Windows. WSL or a Unix based virtual machine might help. Good luck though.

## Contribution:

Throughout the source code, there are comments tagged with TODOs indicating potential areas for improvement, unimplemented features, or optimizations. To find them run the given script inside moOS directory.
```
./find_todos.sh
```
