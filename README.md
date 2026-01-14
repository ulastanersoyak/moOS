# moos

a 32-bit operating system written in c and assembly. it boots with its own
bootloader or with limine, runs preemptively scheduled ring 3 programs from a
fat16 disk with long file names and comes with a user space shell that has
pipes, redirection, job control, history and tab completion. the scheduler
does round robin, fixed priority real time with priority inheritance and
earliest deadline first.

![moOS booting into the shell](images/boot.png)

## features

boot
- its own two stage bootloader (fat16 boot sector, kernel in the reserved
  sectors) or limine through multiboot 1, picked at build time
- gdt and tss built in c, a20, remapped pics

kernel
- cpu exception and pic irq handling, faulting user programs are killed
  without taking down the kernel
- paging with one address space per program, kernel pages are supervisor
  only and program code is read only
- kernel heap with block allocation, usage stats and invalid free detection
- preemptive multitasking with three scheduling policies, switchable at
  runtime with `sched rr` / `sched rt` / `sched edf`:
  - round robin: every task gets a 50 ms time slice in turn
  - real time: fixed priority preemptive. the highest priority ready task
    always runs and preempts lower priority tasks the moment it wakes up,
    equal priorities share the cpu round robin
  - earliest deadline first: periodic tasks run in the order of their
    deadlines, the shell still goes first
- periodic tasks (`period`, `wait_period`) that count their missed deadlines
- named mutexes shared between programs, with optional priority inheritance
  against priority inversion
- an init thread that starts the shell and starts a new one when it exits or
  crashes
- elf loader, `argc`/`argv`, per program heap (`sbrk`)
- `int 0x80` system calls for files, directories, pipes, processes, time and
  the terminal. every pointer coming from user space is checked
- pipes, stdin/stdout/stderr and reference counted file descriptors

drivers
- vga text terminal with scrolling and a hardware cursor
- keyboard (us layout, shift, caps lock, ctrl, arrow keys)
- pit timer, cmos real time clock, serial port (com1)
- ata disk reads and writes, mbr partitions

file system
- fat16: read, create, write, append, truncate, delete, `mkdir`, `rmdir`,
  directory listing, real file dates. images stay valid for `fsck.fat`
- vfat long file names up to 63 chars with 8.3 aliases (`LONGFI~1.TXT`),
  case insensitive lookups and the case of short names kept
- paths relative to a working directory, `.` and `..`

shell
- a user program (`0:/bin/sh.elf`) on top of system calls, a crash in it
  cant take the kernel down
- builtins: `help`, `cd`, `pwd`, `touch`, `rm`, `mkdir`, `run`, `ps`,
  `kill`, `sched`, `prio`, `mem`, `uptime`, `sleep`, `clear`, `reboot`,
  `exit`
- pipelines (`cat notes.txt | sort | wc`), `< file`, `> file`, `>> file`, `&` and
  "quoted arguments"
- job control: `prog &` runs in the background, `ps`, `kill <pid|name>`,
  `prio <1-29> prog` sets the real time priority (default 10, the shell runs
  at 30 so a runaway program cant lock out the console). ctrl+c stops the
  foreground pipeline, ctrl+d ends input typed into a program
- line editing with the arrow keys, home, end and delete, history with
  up/down, tab completion of commands and paths

## screenshots

long file names, redirection and pipelines

![files and pipelines](images/files.png)

a program killed by a page fault, and priority inversion with and without
priority inheritance

![tasks and priority inheritance](images/tasks.png)

periodic tasks under rate monotonic priorities and earliest deadline first

![rate monotonic and edf](images/rtos.png)

the shell builtins

![help](images/help.png)

![cowsay and moose](images/moose.png)

## getting started

requirements:
- clang (or an i686-elf-gcc cross-compiler)
- binutils (ld, objcopy)
- nasm
- qemu
- mtools, dosfstools (mkfs.fat) and util-linux (sfdisk)
- make and a host c compiler (for limine's installer)

limine is a git submodule, clone with `--recursive` or fetch it afterwards:
```bash
git submodule update --init
```

to run:
```bash
make run
```

### bootloaders
moos boots with its own bootloader or with [limine](https://codeberg.org/Limine/Limine),
picked at build time:
```bash
make run                      # moos bootloader (default)
make run BOOTLOADER=limine    # limine
```

| | moos bootloader | limine |
|---|---|---|
| image | `bin/os-moos.img` | `bin/os-limine.img` |
| disk layout | fat16 without a partition table, the kernel sits in its reserved sectors | mbr with one fat16 partition at 1 mb, limine's stages in the mbr and the gap after it |
| kernel | `bin/kernel.bin`, flat binary loaded to 1 mb | `/boot/moos.elf`, booted through multiboot 1 (limine's own protocol is 64 bit only) |

both use the same kernel: `bin/kernel.elf` starts with an entry stub that
works for a plain jump to 1 mb as well as for a multiboot loader, and the
disk driver finds the fat16 partition on its own. the limine config is
`src/bootloader/limine.conf`. to update limine:
```bash
git submodule update --remote external/limine
```

to build with the gcc cross-compiler instead of clang:
```bash
make CC=~/opt/cross/bin/i686-elf-gcc LD=~/opt/cross/bin/i686-elf-ld
```

### tests
the tests boot qemu headless, type commands into the shell through the qemu
monitor and check the kernel log written to qemu's debug console. afterwards
the disk image is checked with `fsck.fat` and the files moos wrote are read
back with mtools:
```bash
make test                     # moos bootloader, or the one picked above
make test-all                 # both bootloaders
```

## user programs

user programs live in `src/user/` and end up in `0:/bin` on the disk image.
type their name in the shell to run them:

| program | does |
|---|---|
| `ls [dir]...` | lists directories with sizes and dates |
| `echo <text>...` | prints its arguments |
| `cat [file]...` | prints files or stdin |
| `grep <word> [file]...` | prints lines containing a word |
| `sort [file]` | prints lines sorted |
| `wc [file]...` | counts lines, words and bytes |
| `cp <from> <to>` | copies a file |
| `date` | prints the date and time of the real time clock |
| `time <program> [args]` | runs a program and prints how long it took |
| `pipetest` | feeds `sort` through a pipe with `pipe`, `spawn` and `wait` |
| `hello [name]...` | greets you, asks for your name without arguments |
| `count` | counts to 10 using the timer |
| `rtdemo` | periodic task that measures its wakeup lateness |
| `spin` | busy loop, a cpu hog for scheduler tests |
| `memtest` | stress tests the user heap |
| `crash [code]` | writes to kernel memory (or its own code) and gets killed |
| `pidemo [nopi]` | priority inversion with and without priority inheritance |
| `edfdemo` | two periodic tasks that only edf can schedule |
| `cowsay [text]` | a cow says something |
| `moose [text]` | the moOS moose |
| `sh` | the shell, started by init at boot |

programs that read stdin when they get no file arguments work in pipelines:
```
moos 0:/> cat dir/long.txt | grep 059 | sort > found.txt
moos 0:/> wc < found.txt
11 22 110
```

to add one, create `src/user/<name>.c` with a `main (int argc, char **argv)`
and add its name to the list of user programs in the makefile. the system
calls and the small libc are in `src/user/lib/moos.h`.

### scheduler demo
`rtdemo` is a periodic task that wakes every 70 ms and reports how late its
wakeups were. run it next to cpu hogs under both policies:
```
moos 0:/> spin &
moos 0:/> spin &
moos 0:/> rtdemo
rtdemo: max lateness 90 ms, average 45 ms
moos 0:/> sched rt
moos 0:/> prio 25 rtdemo
rtdemo: max lateness 0 ms, average 0 ms
moos 0:/> kill spin
```

`pidemo` shows priority inversion. a low priority task holds a lock that a
high priority task needs while a medium priority task hogs the cpu. with
priority inheritance (the default) low runs at high's priority until it
unlocks, with `pidemo nopi` high has to wait for medium too:
```
moos 0:/> pidemo
high: got the lock after 150 ms
moos 0:/> pidemo nopi
high: got the lock after 550 ms
```

`edfdemo` runs two periodic tasks that need 90% of the cpu (30 ms every
60 ms and 36 ms every 90 ms). rate monotonic priorities cant schedule them,
earliest deadline first can:
```
moos 0:/> sched rt
moos 0:/> edfdemo
slow: 36 ms every 90 ms, 9 of 20 deadlines missed
moos 0:/> sched edf
moos 0:/> edfdemo
slow: 36 ms every 90 ms, 0 of 20 deadlines missed
```

## project structure
- `src/bootloader/` - boot sector, kernel entry stub and limine config
- `src/kernel/` - core os: interrupts, memory, tasks, scheduler, mutexes,
  syscalls
- `src/drivers/` - screen, keyboard, timer, clock, serial and disk drivers
- `src/file_system/` - vfs, path parser and fat16 driver
- `src/libc/` - kernel side c library
- `src/user/` - user programs including the shell, and their syscall
  library
- `external/limine/` - limine bootloader (git submodule)
- `scripts/` - qemu test driver and helpers
- `docs/` - debugging guide and fat16 reference

## installation

### arch linux
```bash
sudo pacman -S git qemu make nasm clang mtools dosfstools util-linux
```

### fedora
```bash
sudo dnf install git qemu-system-x86-core qemu-ui-gtk make nasm clang mtools dosfstools util-linux
```

for an i686-gcc cross-compiler instead of clang, follow [this guide](https://wiki.osdev.org/GCC_Cross-Compiler)

## license
see license file for details
