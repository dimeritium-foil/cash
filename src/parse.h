#ifndef PARSE_H
#define PARSE_H

#include <unistd.h>

char** parse_input();
char*** separate_inputs(char** input);

char* del_char(char *str, char garbage_char);
int char_occurrences(char *str, char chr);

void add_bg_process(pid_t cpid, char** argv);
void remove_bg_process(pid_t cpid);

#endif
