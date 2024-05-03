## Table of Contents
1. [introduction](#introduction)
2. [installation](#installation)
    - [linux](#linux)
    - [mac](#mac)
    - [windows](#windows)
3. [contribution](#contribution)

<a name="introduction"></a>
## Introduction
moOS is a 32-bit operating system project in its early stages, featuring a dedicated bootloader, kernel, and standard library. Presently, the system is at a foundational stage, not yet offering specific functionalities, but the groundwork has been laid down.

![Screenshot from 2023-11-05 17-51-21](https://github.com/ulastanersoyak/moOS/assets/92662728/c7377806-eaa6-427b-b570-86b314c03937)

## installation:

### linux:
lo work with moOS on Linux, ensure you have the following dependencies installed:

ubuntu/debian:
- git: `sudo apt-get install git` 
- qemu: `sudo apt-get install qemu-system-x86`
- make: `sudo apt-get install make`
- nasm: `sudo apt-get install nasm`

  
arch:
- git: `sudo pacman -S git` 
- qemu: `sudo pacman -S qemu`
- make: `sudo pacman -S make`
- nasm: `sudo pacman -S nasm`

i686-gcc cross-compiler and linker: Follow [this guide](https://wiki.osdev.org/GCC_Cross-Compiler).

once dependencies are set, clone the repository and compile the code:
```bash
git clone git@github.com:ulastanersoyak/moOS.git
cd ./moOS
make # To compile the code
make run # To run moOS in QEMU
./run.sh  # To clean directories, recompile, and run moOS in QEMU
```

### mac:

steps above will **probably** work fine.

### windows:

i have no idea how you can run it on Windows. WSL or a Unix based virtual machine might help. good luck though.
