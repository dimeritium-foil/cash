#ifndef MAIN_H
#define MAIN_H

#include <unistd.h>

#define MAX_SIZE 256
#define MAX_PROMPT_SIZE  16384
#define FILLER_LINE_SIZE 8129
#define MAX_BG_PROC 64

typedef struct BgProcess {
    pid_t pid;
    char** command;
} BgProcess;

void free_input(char** input);
void free_array_of_inputs(char*** array_of_inputs);

char* generate_prompt();
void truncate_dir(char* dir_name, int truncate_length);
void print_greeting();

#endif
