#ifndef SH_H
#define SH_H

#include "lib/moos.h"

#define SH_LINE_MAX 128

// completions are file names plus a trailing / for directories
#define SH_COMPLETION_LEN (NAME_MAX + 2)
#define SH_MAX_COMPLETIONS 32

// navigation keys come from the terminal as codes above the ascii range,
// must match src/drivers/keyboard/keyboard.h
#define KEY_CTRL_C 0x03
#define KEY_CTRL_D 0x04
#define KEY_UP 0x80
#define KEY_DOWN 0x81
#define KEY_LEFT 0x82
#define KEY_RIGHT 0x83
#define KEY_HOME 0x84
#define KEY_END 0x85
#define KEY_DELETE 0x86

// prints the prompt with the working directory
void sh_print_prompt (void);

// fills out with the candidates for the word prefix (prefix_len chars). a
// word at the start of the line is completed as a command, other words as
// paths. name_start is set to where the completed part of the word starts
// (after the last /). returns number of candidates
int sh_complete (const char *prefix, uint32_t prefix_len, uint8_t is_command,
                 uint32_t *name_start, char out[][SH_COMPLETION_LEN], int max);

// reads a line into line (at most max - 1 chars). supports editing with the
// arrow keys, home, end, delete and backspace, history with up and down and
// completion with tab. ctrl+c drops the line
void sh_readline (char *line, uint32_t max);

#endif // !SH_H
