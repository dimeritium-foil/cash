#ifndef MAIN_H
#define MAIN_H

void free_input(char** input);
void free_array_of_inputs(char*** array_of_inputs);

char* generate_prompt();
void truncate_dir(char* dir_name, int truncate_length);
void print_greeting();

#endif
