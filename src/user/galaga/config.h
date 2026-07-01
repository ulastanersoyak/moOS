// what autoconf would find out about moos for xgalaga, included in front of
// every one of its files
#define PACKAGE "xgalaga-sdl"
#define VERSION "2.1.1.0-sdl"
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_STRINGS_H 1
// there is no sound driver
#define NOSOUND 1
// where the makefile puts images/ and levels/ on the disk image
#define DATADIR "0:/galaga"

// the c library has no atexit, src/user/galaga.c has one that never calls
// the functions: exiting brings back the text screen on its own
int atexit (void (*function) (void));
