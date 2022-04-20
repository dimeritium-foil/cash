#ifndef PARSE_H
#define PARSE_H

char** parse_input();
char*** separate_inputs(char** input);

char* del_char(char *str, char garbage_char);
int char_occurrences(char *str, char chr);

#endif
