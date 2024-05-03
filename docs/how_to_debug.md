### how to debug the kernel with qemu and gdb

- run gdb on the bin folder 

```
cd bin/
gdb
```

- load the debug symbols to gdb starting from .text_addr 0x100000

```
add-symbol-file ../build/kernelfull.o 0x100000
```
- start a remote debug and pipe it to the qemu
```
target remote | qemu-system-i386 -hda ./os.bin -gdb stdio
```

sources:

- https://wiki.osdev.org/QEMU
- https://wiki.osdev.org/GDB
