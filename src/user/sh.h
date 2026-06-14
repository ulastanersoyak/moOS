#ifndef SH_H
#define SH_H

#include "lib/moos.h"

#define SH_LINE_MAX 128

// completions are file names plus a trailing / for directories
#define SH_COMPLETION_LEN (NAME_MAX + 2)
#define SH_MAX_COMPLETIONS 32

// ctrl+c and ctrl+d typed on the terminal, the navigation keys come as the
// KEY_ codes from lib/moos.h
#define KEY_CTRL_C 0x03
#define KEY_CTRL_D 0x04

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

// text the next sh_readline starts with, as if it was typed
void sh_type_ahead (const char *text);

#endif // !SH_H
