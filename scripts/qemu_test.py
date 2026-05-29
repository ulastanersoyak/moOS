#!/usr/bin/env python3
"""boots moOS headless, types commands into the shell through the qemu
monitor and checks the kernel log written to the debug console.

usage: qemu_test.py [disk image], defaults to bin/os-moos.img"""

import os
import re
import shutil
import subprocess
import sys
import time

# the shell tests write to the disk, so they run on a copy of the image
SOURCE_IMAGE = sys.argv[1] if len(sys.argv) > 1 else "./bin/os-moos.img"
IMAGE = "./build/test.img"
PARTITION_COPY = "./build/test-part.img"
LOG = "./build/debugcon.log"

# chars that need a qemu sendkey name
KEY_NAMES = {
    " ": "spc", "\n": "ret", ".": "dot", "/": "slash", "-": "minus",
    ":": "shift-semicolon", "_": "shift-minus", ",": "comma", "&": "shift-7",
    ">": "shift-dot", "<": "shift-comma", "|": "shift-backslash",
    '"': "shift-apostrophe", "~": "shift-grave_accent",
}


def key_name(c):
    if c in KEY_NAMES:
        return KEY_NAMES[c]
    if c.isupper():
        return "shift-" + c.lower()
    return c


# (text to type or special key, seconds to wait afterwards)
STEPS = [
    ("mem\n", 0.5),
    ("help\n", 0.5),
    ("echo hi there\n", 0.3),
    ("ls\n", 0.5),
    ("ls dir\n", 0.5),
    ("ls nosuch\n", 0.5),
    ("cat fs_test.txt\n", 0.5),
    ("cat /dir/long.txt\n", 2.5),
    ("uptime\n", 0.3),
    ("sleep 300\n", 0.6),
    ("hello\n", 0.5),
    ("Tan\n", 0.5),
    ("count\n", 1.3),
    ("<ctrl-c>", 0.5),
    ("crash\n", 0.8),
    ("run 0:/bin/count.elf\n", 6.0),
    ("hello\n", 0.5),
    ("<ctrl-c>", 0.5),
    ("spin\n", 1.0),
    ("<ctrl-c>", 0.5),
    ("foo\n", 0.3),
    # scheduler: two cpu hogs in the background, then a periodic task under
    # both policies
    ("spin &\n", 0.3),
    ("spin &\n", 0.3),
    ("ps\n", 0.8),
    ("rtdemo\n", 3.5),
    ("sched rt\n", 0.3),
    ("prio 25 rtdemo\n", 3.0),
    # a hog above everything else still cant lock out the shell
    ("prio 29 spin &\n", 0.5),
    ("ps\n", 0.5),
    # background tasks dont get keyboard input, kill has to wake them up
    ("hello &\n", 0.5),
    ("kill spin\n", 0.5),
    ("kill hello\n", 0.5),
    ("sched rr\n", 0.3),
    ("ps\n", 0.5),
    # elf programs with arguments, heap, file syscalls and redirection
    ("hello Tan Bob\n", 0.5),
    ("<up>", 0.3),
    ("\n", 0.5),
    ("memtest\n", 3.5),
    ("fputest\n", 4.0),
    ("libctest\n", 1.0),
    # graphics mode, left normally and by getting killed
    ("gfxdemo\n", 3.6),
    ("gfxdemo\n", 1.0),
    ("<ctrl-c>", 0.6),
    ("echo back to text\n", 0.4),
    # key events: every key down and up, also ones without a char
    ("keys\n", 0.6),
    ("a", 0.3),
    ("<shift-b>", 0.3),
    ("<up>", 0.3),
    ("<ctrl>", 0.3),
    ("<esc>", 0.5),
    ("crash code\n", 0.6),
    ("echo banana > fruit.txt\n", 0.4),
    ("echo apple >> fruit.txt\n", 0.4),
    ("cat fr", 0.2),
    ("<tab>", 0.3),
    ("\n", 0.5),
    ("sort fruit.txt > sorted.txt\n", 0.6),
    ("mkdir notes\n", 0.4),
    ("cd notes\n", 0.3),
    ("cp ../fruit.txt f.txt\n", 0.6),
    ("wc f.txt\n", 0.5),
    ("pwd\n", 0.3),
    ("cd ..\n", 0.3),
    ("rm notes\n", 0.4),
    ("rm nothere\n", 0.4),
    ("cat dir/long.txt > copy.txt\n", 1.5),
    ("ls\n", 0.5),
    # long file names, looked up ignoring case or through the 8.3 alias
    ('echo "apple banana" > "long notes about moos.txt"\n', 0.5),
    ('mkdir "My Projects"\n', 0.4),
    ('cd "my projects"\n', 0.3),
    ("echo hello > ReadMe.md\n", 0.4),
    ("ls\n", 0.5),
    ("cd ..\n", 0.3),
    ('ls | grep long\n', 0.6),
    ("cat LONGNO~1.TXT\n", 0.5),
    ('rm "my projects/readme.md" "my projects"\n', 0.6),
    # pipes, < redirection, programs starting programs
    ("cat fruit.txt|sort\n", 0.6),
    ('echo "two  spaces" | wc\n', 0.6),
    ("ls bin | grep elf | sort | wc\n", 1.0),
    ("cat dir/long.txt | sort | grep 600 | wc\n", 1.5),
    ("wc < fruit.txt\n", 0.5),
    ("cat nosuch | wc\n", 0.6),
    ("pipetest\n", 0.8),
    ("time sort fruit.txt\n", 0.6),
    ("spin | wc\n", 0.6),
    ("<ctrl-c>", 0.6),
    ("count | wc &\n", 6.5),
    ("wc\n", 0.4),
    ("typed line\n", 0.3),
    ("<ctrl-d>", 0.5),
    ("cd | sort\n", 0.3),
    ("date\n", 0.4),
    # priority inheritance, and periodic tasks under rate monotonic
    # priorities and earliest deadline first
    ("pidemo\n", 1.5),
    ("pidemo nopi\n", 1.5),
    ("sched rt\n", 0.3),
    ("edfdemo\n", 2.5),
    ("sched edf\n", 0.3),
    ("edfdemo\n", 2.5),
    # the shell runs before periodic tasks, so ps can make them late
    ("edfdemo &\n", 0.8),
    ("ps\n", 2.0),
    ("sched rr\n", 0.3),
    # doom: start it, quit through the menu (esc, up to quit game, enter, y)
    ("doom\n", 12.0),
    ("<esc>", 1.0),
    ("<up>", 0.5),
    ("\n", 1.0),
    ("y", 4.0),
    ("echo doom is done\n", 0.4),
    # the desktop: file menu, new folder in the open root window, ctrl+q.
    # the mouse starts in the middle of the screen
    ("desk\n", 3.0),
    ("!mouse_move -100 -100", 0.05),
    ("!mouse_move -100 -100", 0.05),
    ("!mouse_move -88 -30", 0.3),
    ("!mouse_button 1", 0.3),
    ("!mouse_move 8 42", 0.3),
    ("!mouse_button 0", 1.0),
    # new file opens the editor: insert a line, copy it with yyp, save with
    # :wq
    ("!mouse_move -8 -42", 0.3),
    ("!mouse_button 1", 0.3),
    ("!mouse_move 8 62", 0.3),
    ("!mouse_button 0", 1.0),
    ("ihello from the editor", 0.3),
    ("<esc>", 0.3),
    ("yyp:wq\n", 1.0),
    # a shell window from the moose menu writes a file
    ("!mouse_move -34 -62", 0.3),
    ("!mouse_button 1", 0.3),
    ("!mouse_move 14 102", 0.3),
    ("!mouse_button 0", 1.5),
    ("echo made in a window > win.txt\n", 1.0),
    ("<ctrl-q>", 0.8),
    ("ls\n", 0.5),
    ("cat Untitled.txt\n", 0.5),
    ("cat win.txt\n", 0.5),
    # the shell is a user program, init starts a new one when it dies
    ("kill sh\n", 1.0),
    ("ps\n", 0.5),
    ("mem\n", 0.5),
]

EXPECTED = [
    "system paging [OK]",
    "pit timer init <100 hz> [OK]",
    "welcome to moOS!",
    "cd [dir]",                       # help
    "hi there",                       # echo
    "fs_test.txt   14 bytes",         # ls
    "dir/          <dir>",
    "long.txt      6000 bytes",       # ls dir
    "nosuch: not found",
    "file sys text",                  # cat
    "line 0600",
    "ticks)",                         # uptime
    "hello from ring 3!",             # hello reads input via syscall
    "nice to meet you, Tan",
    "counting to 10",                 # count gets killed with ctrl+c
    "exception: page fault",          # crash writes to kernel memory
    "[killed]",
    "1 2 3 4 5 6 7 8 9 10\n[exit code 10]",
    "spinning forever, press ctrl+c to stop\n^C\n[killed]",
    "foo: unknown command",
    "scheduler: real time (fixed priority preemptive)",
    "scheduler: round robin",
    "spin killed",
    "hello killed",
    "nice to meet you, Tan\nnice to meet you, Bob",
    "memtest: ok",
    "fputest: ok",                    # tasks keep their own fpu state
    "libctest: ok",
    " adding 0:/doom/doom1.wad",
    "I_InitGraphics: Auto-scaling factor: 1\nmoos 0:/> echo doom is done\ndoom is done",
    "moos 0:/> gfxdemo\n^C\n[killed]\nmoos 0:/> echo back to text\nback to text",
    "down a\nup a\ndown shift\ndown b\nup b\nup shift\ndown up\nup up\n"
    "down ctrl\nup ctrl\ndown esc\nmoos 0:/>",
    "writing to my own code...\nexception: page fault",
    "moos 0:/> cat fruit.txt \nbanana\napple",   # tab completion
    "2 2 13 f.txt",
    "0:/notes",
    "notes: directory not empty",
    "nothere: not found",
    "sorted.txt    13 bytes",
    "copy.txt      6000 bytes",
    "long notes about moos.txt  13 bytes",   # vfat long names
    "ReadMe.md     6 bytes",
    "moos 0:/> cat LONGNO~1.TXT\napple banana",
    "moos 0:/> cat fruit.txt|sort\napple\nbanana\n",
    "1 2 12\n",                    # quotes keep both spaces
    "27 135 1215\n",               # 27 programs through a 4 stage pipeline
    "1 2 10\n",                    # 6000 bytes through pipes larger than 4 kb
    "2 2 13\n",
    "nosuch: not found\n0 0 0",
    "apple\nbanana\nfig\npear\npipetest: sort exited with 0",
    "time: ",
    "moos 0:/> spin | wc\n^C\n[killed]",
    "count done",
    "wc done",
    "typed line\n1 2 11",          # terminal input ended with ctrl+d
    "cd: pipes, redirection and & only work with programs",
    "init: sh was killed, starting a new one\n\nwelcome to moOS!",
    "scheduler: earliest deadline first",
    "ps/2 mouse init [OK]",
    "New Folder/",                     # made in the desktop
    "hello from the editor\nhello from the editor\n",
    "moos 0:/> cat win.txt\nmade in a window\n",   # the shell window
    "every 60 ms, 0 of",               # ps shows periodic tasks
]

FORBIDDEN = ["kernel panic", "still alive", "[ER]", "kfree: invalid"]


# byte offset of the first fat16 partition, 0 for an unpartitioned disk (the
# moOS bootloader layout). same check as disk_find_fat_partition in disk.c
def fat_offset():
    sector = open(IMAGE, "rb").read(512)
    if sector[510:512] != b"\x55\xaa":
        return 0
    for i in range(4):
        entry = sector[0x1BE + i * 16:0x1BE + (i + 1) * 16]
        lba = int.from_bytes(entry[8:12], "little")
        if entry[0] in (0x00, 0x80) and entry[4] in (0x04, 0x06, 0x0E) and lba:
            return lba * 512
    return 0


def mtools(*args):
    return subprocess.run(["mtype", "-i", f"{IMAGE}@@{fat_offset()}", *args],
                          capture_output=True,
                          env={"MTOOLS_SKIP_CHECK": "1", "PATH": "/usr/bin:/bin"})


# the image written by moOS must be a valid fat16 file system that other
# tools can read
def check_disk():
    failed = []
    # fsck.fat cant start at an offset, check a copy of the partition
    with open(IMAGE, "rb") as disk, open(PARTITION_COPY, "wb") as part:
        disk.seek(fat_offset())
        part.write(disk.read())
    fsck = subprocess.run(["fsck.fat", "-n", PARTITION_COPY],
                          capture_output=True, text=True)
    if fsck.returncode != 0:
        failed.append("fsck.fat found errors:\n" + fsck.stdout)
    if mtools("::/sorted.txt").stdout != b"apple\nbanana\n":
        failed.append("sorted.txt has wrong content")
    if mtools("::/copy.txt").stdout != open("./build/long.txt", "rb").read():
        failed.append("copy.txt differs from long.txt")
    if mtools("::/long notes about moos.txt").stdout != b"apple banana\n":
        failed.append("long notes about moos.txt has wrong content")
    return failed


# boot time depends on the bootloader (limine shows its menu for a second)
def wait_for_shell(timeout=30):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            if "welcome to moOS" in open(LOG, encoding="latin-1").read():
                time.sleep(0.3)
                return
        except FileNotFoundError:
            pass
        time.sleep(0.2)
    raise RuntimeError("moOS did not reach the shell")


def main():
    shutil.copyfile(SOURCE_IMAGE, IMAGE)
    # a log left from the last run would look like a finished boot
    if os.path.exists(LOG):
        os.remove(LOG)
    qemu = subprocess.Popen(
        ["qemu-system-i386", "-drive", f"file={IMAGE},format=raw",
         "-rtc", "base=localtime",
         # without the vmmouse the mouse is relative, like the monitor's
         # mouse_move
         "-machine", "vmport=off",
         "-display", "none", "-debugcon", f"file:{LOG}", "-monitor", "stdio",
         "-no-reboot"],
        stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, text=True)
    try:
        wait_for_shell()
        for text, wait in STEPS:
            # a qemu monitor command, e.g. to move the mouse
            if text.startswith("!"):
                qemu.stdin.write(text[1:] + "\n")
                qemu.stdin.flush()
                time.sleep(wait)
                continue
            keys = [text[1:-1]] if text.startswith("<") else map(key_name, text)
            for key in keys:
                qemu.stdin.write(f"sendkey {key}\n")
                qemu.stdin.flush()
                time.sleep(0.05)
            time.sleep(wait)
        qemu.stdin.write("quit\n")
        qemu.stdin.flush()
        qemu.wait(timeout=5)
    finally:
        if qemu.poll() is None:
            qemu.kill()

    log = open(LOG, encoding="latin-1").read()
    print(log)
    failed = [e for e in EXPECTED if e not in log]
    failed += [f"forbidden: {f}" for f in FORBIDDEN if f in log]
    # ctrl+c has to stop count while it sleeps, before it reaches 10
    if not re.search(r"press ctrl\+c to stop\n(\d )*\d ?\^C\n\[killed\]", log):
        failed.append("count was not killed by ctrl+c")
    if log.count("[exit code 10]") != 1:
        failed.append("count should exit normally exactly once")
    if "what's your name? ^C\n[killed]" not in log:
        failed.append("hello was not killed by ctrl+c while reading input")
    # under round robin the periodic task waits for the hogs' time slices,
    # under the real time policy it preempts them as soon as it wakes up
    late = [int(x) for x in re.findall(r"rtdemo: max lateness (\d+) ms", log)]
    if len(late) != 2 or late[0] < 40 or late[1] > 10:
        failed.append(f"unexpected rtdemo lateness (rr, rt): {late}")
    # both hogs must have shared the cpu while the shell was idle
    hogs = re.findall(r"\n(\d+) +spin +(?:ready|running) +10 +(\d+)\.(\d)(\d) s &", log)
    if len(hogs) < 4:
        failed.append(f"spin tasks missing from ps: {hogs}")
    # after kill only kernel threads may be left
    last_ps = log[log.rfind("pid  name"):]
    if "spin" in last_ps or "rtdemo" in last_ps:
        failed.append("tasks left over after kill")
    # with priority inheritance high only waits for low's critical section,
    # without it mid runs first
    waits = [int(x) for x in re.findall(r"high: got the lock after (\d+) ms", log)]
    if len(waits) != 2 or waits[0] > 300 or waits[1] < waits[0] + 150:
        failed.append(f"unexpected pidemo waits (pi, no pi): {waits}")
    # rate monotonic misses deadlines of slow, edf misses none
    slow = [int(x) for x in re.findall(r"slow: 36 ms every 90 ms, (\d+) of 20", log)]
    fast = re.findall(r"fast: 30 ms every 60 ms, (\d+) of 30", log)
    if len(slow) != 3 or slow[0] == 0 or slow[1] != 0 or fast[:2] != ["0", "0"]:
        failed.append(f"unexpected edfdemo misses (rt, edf): {slow} {fast}")
    if not re.search(r"gfxdemo: [1-9]\d* frames in 3\d\d\d ms", log):
        failed.append("gfxdemo did not draw any frames")
    # running and killing programs must not leak heap blocks
    mem = re.findall(r"heap: \d+ kb used / \d+ kb total \((\d+) of", log)
    if len(mem) != 2 or mem[0] != mem[1]:
        failed.append(f"heap usage changed across programs: {mem}")
    # files written by moOS carry the date of the real time clock
    today = time.strftime("%Y-%m-%d")
    if not re.search(rf"fruit\.txt +13 bytes +{today} ", log):
        failed.append(f"new files are not dated {today}")
    if log.count("nice to meet you, Bob") != 2:
        failed.append("history did not repeat the last command")
    failed += check_disk()
    if failed:
        print("FAILED:")
        for f in failed:
            print("  " + repr(f))
        return 1
    print("all shell tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
