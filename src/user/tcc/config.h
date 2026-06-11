// tcc's build configuration for moOS, normally written by its configure
// script. tcc makes static i386 elf programs for moos

#define TCC_VERSION "0.9.28rc"
#define TCC_TARGET_I386 1

// no shared libraries, programs link against /lib/libc.a
#define CONFIG_TCC_STATIC 1
#define CONFIG_TCC_BACKTRACE 0
#define CONFIG_TCC_BCHECK 0
#define CONFIG_TCC_SEMLOCK 0
// tccdefs.h is built into tcc, see the makefile
#define CONFIG_TCC_PREDEFS 1

// paths on the moos disk. a path starting with / stays on the drive of the
// working directory. the cross compiler on the host sets CONFIG_SYSROOT to
// a copy of these directories
#ifndef CONFIG_SYSROOT
#define CONFIG_SYSROOT ""
#endif
#define CONFIG_TCCDIR CONFIG_SYSROOT "/lib/tcc"
#define CONFIG_TCC_SYSINCLUDEPATHS "{B}/include:" CONFIG_SYSROOT "/include"
#define CONFIG_TCC_LIBPATHS "{B}:" CONFIG_SYSROOT "/lib"
#define CONFIG_TCC_CRTPREFIX CONFIG_SYSROOT "/lib"
