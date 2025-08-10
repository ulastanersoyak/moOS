### how to debug the kernel with qemu and gdb

- build the image you want to debug (`make` or `make BOOTLOADER=limine`)

- start gdb with the kernel symbols. `bin/kernel.elf` is linked at its load
  address, no offset is needed

```
gdb bin/kernel.elf
```

- start qemu from gdb and connect to its gdb stub

```
target remote | qemu-system-i386 -drive file=bin/os-moos.img,format=raw -gdb stdio
```

- user programs are separate elf files in `build/user/`. they are stripped
  when linked; to debug one, link it without `-s` and load its symbols with
  `add-symbol-file build/user/<name>.elf`

sources:

- https://wiki.osdev.org/QEMU
- https://wiki.osdev.org/GDB
