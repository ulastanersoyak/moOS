#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

#include <stdint.h>

#define SHELL_LINE_MAX 128

// completions are names of at most 8.3 chars plus a trailing / for
// directories
#define SHELL_COMPLETION_LEN 16
#define SHELL_MAX_COMPLETIONS 32

// prints the prompt with the working directory
void shell_print_prompt (void);

// fills out with the candidates for the word prefix (prefix_len chars). a
// word at the start of the line is completed as a command, other words as
// paths. name_start is set to where the completed part of the word starts
// (after the last /). returns number of candidates
int shell_complete (const char *prefix, uint32_t prefix_len,
                    uint8_t is_command, uint32_t *name_start,
                    char out[][SHELL_COMPLETION_LEN], int max);

// reads a line into line (at most max - 1 chars). supports editing with the
// arrow keys, home, end, delete and backspace, history with up and down and
// completion with tab. ctrl+c drops the line
void shell_readline (char *line, uint32_t max);

#endif // !SHELL_INTERNAL_H
