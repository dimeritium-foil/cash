#ifndef EXECUTE_H
#define EXECUTE_H

void execute_input(char** input);
int execute_builtin(char** input);
void execute_piped_inputs(char*** array_of_inputs);

void redirect_io(char* filename, int io_type, int append_flag);
int handle_io_redirection(char** input);

void sigchld_handler(int sig, siginfo_t *info, void *context);

#endif
