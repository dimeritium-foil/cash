#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "execute.h"
#include "parse.h"

#include "globals.h"

#define PREAD  0
#define PWRITE 1

void execute_input(char** input) {
    /*
     * Takes a parsed array of strings and performs the according fork-exec
     *
     * Arguments:
     *  input: A null terminated array of char pointers (strings)
     */

    // Return if the input is empty
    if (input == NULL)
        return;
    else if (input[0] == NULL)
        return;

    if (execute_builtin(input))
        return;

    int input_len;

    // Index of the last argument (bec the actual last element in this vector is NULL)
    int last_arg = 0;
    while (input[last_arg] != NULL)
        last_arg++;
    input_len = last_arg;
    last_arg--;

    if (last_arg == 0) {
        fprintf(stderr, "%serror%s: no command supplied\n", colors[ERR_COLOR], color_reset);
        return;
    }

    // Flag that decides whether to run the command in the background or not
    int run_in_background = 0;

    // If a '&' is provided as the last argument, set up command to run in the background
    if (strcmp(input[last_arg], "&") == 0) {

        // Erase the '&' sign
        free(input[last_arg]);
        input[last_arg] = NULL;

        // Set the flag
        run_in_background = 1;

        // Set up signal handler for when the background process terminates
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = sigchld_handler;
        sigemptyset(&sa.sa_mask);

        sigaction(SIGCHLD, &sa, NULL);

    }

    // Create a child proces that is a fork (clone) of the current one
    pid_t fork_pid = fork();

    if (fork_pid < 0) {
        // Fork failed
        fprintf(stderr, "%serror%s: fork failed\n", colors[ERR_COLOR], color_reset);
        exit(1);
    } 
    else if (fork_pid == 0) {
        // If the fork is successful, create a new process that will overwrite the
        // child process, with any command you run

        // Redirect all I/O to /dev/null
        if (run_in_background) {
            redirect_io("/dev/null", STDOUT_FILENO, 0);
            redirect_io("/dev/null", STDIN_FILENO, 0);
            redirect_io("/dev/null", STDERR_FILENO, 0);
        }
        else {
            int redirect_index = handle_io_redirection(input);
            
            if (redirect_index != -1) {
                for (int i = redirect_index; i < input_len; i++) {
                    free(input[i]);
                    input[i] = NULL;
                }
            }
        }

        execvp(input[0], input);

        // execvp never returns if it failed, so this normaly won't execute
        fprintf(stderr, "%serror%s: command '%s' not found\n", colors[ERR_COLOR], color_reset, input[0]);
        exit(EXIT_FAILURE);
    }
    else {
        // Make the parent process wait for the child process (our command) to finish running
        // if the command is to be ran in the background, we don't wait
        if (!run_in_background)
            waitpid(fork_pid, NULL, 0);
        else {
            rl_save_prompt();
            // printf("[%d] started in the background\n", fork_pid);
            add_bg_process(fork_pid, input);
            rl_restore_prompt();
            
            waitpid(-1, NULL, WNOHANG);
        }
    }
}

int execute_builtin(char** input) {
    /*
     * Takes a parsed array of strings and checks if it's a built in command
     *
     * Arguments:
     *  input: A null terminated array of char pointers (strings)
     *
     *  Returns: 1 if a built-in command was executed, 0 otherwise
     */

    if (strcmp(input[0], "exit") == 0)
        exit(0);
    else if (strcmp(input[0], "cd") == 0) {

        int chdir_code;

        // cd into home directory if no argument is given
        if (input[1] == NULL)
            chdir_code = chdir(getenv("HOME"));
        else
            chdir_code = chdir(input[1]);

        if (chdir_code < 0)
            fprintf(stderr, "%scd error%s: directory not found\n", colors[ERR_COLOR], color_reset);

        return 1;
    }
    else if (strcmp(input[0], "color") == 0) {

        if (input[1] == NULL)
            printf("color: missing operand\nType 'color -h' for proper usage.\n");
        else {
            // Print help message
            if (strcmp(input[1], "-h") == 0) {
                printf("usage: color [color_code]\n\n");
                printf("valid color codes:\n");
                printf("  0 -> red\n");
                printf("  1 -> green\n");
                printf("  2 -> yellow\n");
                printf("  3 -> blue\n");
                printf("  4 -> purple\n");
                printf("  5 -> cyan\n");
                return 1;
            }

            char *end;
            long value = strtol(input[1], &end, 10); 

            // Handle error
            if (end == input[1] || *end != '\0' || errno == ERANGE)
                printf("color: invalid operand\n Type 'color -h' for proper usage.\n");
            else if (value > 5 || value < 0)
                printf("color: invalid operand\n Type 'color -h' for proper usage.\n");
            else
                accent_color = value;
        }

        return 1;
    }
    else if (strcmp(input[0], "jobs") == 0) {

        int count = 0;

        printf("currently running:\n");

        for (int i = 0; i < MAX_BG_PROC; i++) {
            if (bg_processes[i].pid != -1) {
                printf("[%s%d%s] ", colors[PID_COLOR], bg_processes[i].pid, color_reset);

                int j = 0;
                while (bg_processes[i].command[j] != NULL) {
                    printf("%s ", bg_processes[i].command[j]);
                    j++;
                }

                printf("\n");
                count++;
            }
        }

        if (count == 0)
            printf("none\n");

        return 1;
    }
    // Display help message
    else if (strcmp(input[0], "help") == 0) {

        printf("built-in commands:\n");
        printf("  exit: exit the shell\n");
        printf("  cd: change directory\n");
        printf("  color: change the accent color\n");
        printf("  jobs: shows the processes running in the background\n");
        printf("  help: show this message\n\n");

        printf("features:\n");
        printf("  - history, tab completion, and readline keybinds\n");
        printf("  - running processes in the background ('&')\n");
        printf("  - pipes ('|')\n");
        printf("  - I/O redirection ('>', '>>', '<')\n");
        printf("  - string quotes (e.g: \"hello\")\n");

        return 1;
    }

    return 0;
}

void execute_piped_inputs(char*** array_of_inputs) {
    /*
     * Execute each input and redirect it's I/O to the appropriate pipe 
     *
     * Arguments:
     *  array_of_inputs: A NULL terminated array of NULL terminated input arrrays
     */

    // Count the number of inputs
    int inputs_num = 0;
    while (array_of_inputs[inputs_num] != NULL)
        inputs_num++;

    int pipes_num = inputs_num - 1;

    // 2D array of pipes file descriptors
    int** pipes_fds = malloc(sizeof(int*) * pipes_num);
    for (int i = 0; i < pipes_num; i++)
        pipes_fds[i] = malloc(sizeof(int) * 2);

    // Open each pipe
    for (int i = 0; i < pipes_num; i++) {
        if (pipe(pipes_fds[i]) == -1) {
            fprintf(stderr, "%serror%s: pipe failed\n", colors[ERR_COLOR], color_reset);
            return;
        }
    }

    // An array that will contain the PID of each child process
    int* pids = malloc(sizeof(int) * inputs_num);

    // Fork and exec each input
    for (int i = 0; i < inputs_num; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            fprintf(stderr, "%serror%s: fork failed\n", colors[ERR_COLOR], color_reset);
            return;
        }

        // Child process
        if (pids[i] == 0) {

            // Write to the first pipe only if it's the first input in the chain
            if (i == 0) {
                dup2(pipes_fds[0][PWRITE], STDOUT_FILENO);
            }
            // Read from last pipe only if it's the last input in the chain
            else if (i == inputs_num - 1) {
                dup2(pipes_fds[pipes_num - 1][PREAD], STDIN_FILENO);
            }
            // If otherwise, read from the previous pipe, and write to the next pipe
            else {
                dup2(pipes_fds[i - 1][PREAD], STDIN_FILENO);
                dup2(pipes_fds[i][PWRITE], STDOUT_FILENO);
            }

            // Close all pipes since we don't need them after duplication
            for (int j = 0; j < pipes_num; j++) {
                close(pipes_fds[j][PREAD]);
                close(pipes_fds[j][PWRITE]);
            }

            execvp(array_of_inputs[i][0], array_of_inputs[i]);

            // execvp never returns if it failed, so this normaly won't execute
            fprintf(stderr, "%serror%s: command '%s' not found\n", colors[ERR_COLOR], color_reset, array_of_inputs[i][0]);
            exit(EXIT_FAILURE);
        }
    }

    // Close all the parent's pipes, and free the file descriptors for each
    for (int j = 0; j < pipes_num; j++) {
        close(pipes_fds[j][PREAD]);
        close(pipes_fds[j][PWRITE]);

        free(pipes_fds[j]);
    }

    // Wait for each process to terminate
    for (int i = 0; i < inputs_num; i++) {
        if (pids[i] != -1)
            waitpid(pids[i], NULL, 0);
    }

    free(pipes_fds);
    free(pids);
}

void redirect_io(char* filename, int io_type, int append_flag) {
    /*
     * Redirects the specified I/O to the specified file
     *
     * Arguments:
     *  filename: The file to which the I/O will be redirected
     *  io_type: The type of I/O to be redirected (STDOUT_FILENO, STDERR_FILENO, STDIN_FILENO)
     *  append_flag: Specifies whether to overwrite the file or append to it
     */

    FILE* io_file;
    char* mode;

    // If we redirect stdout or stderr we do a write
    if (io_type == STDOUT_FILENO || io_type == STDERR_FILENO) {
        if (append_flag)
            mode = "a";
        else
            mode = "w";
    }
    // Otherwise for stdin we do a read
    else if (io_type == STDIN_FILENO) {
        mode = "r";
    }

    io_file = fopen(filename, mode);

    if (io_file == NULL) {
        fprintf(stderr, "%serror%s: I/O redirection failed\n", colors[ERR_COLOR], color_reset);
        exit(EXIT_FAILURE);
    }

    // Perform the actual redirection
    dup2(fileno(io_file), io_type);
    fclose(io_file);
}

int handle_io_redirection(char** input) {
    /*
     * Searches for the I/O redirection operators ('>', '>>', '<')
     * and performs the according redirection
     *
     * Arguments:
     *  input: A null terminated array of char pointers (strings)
     *
     * Returns: The index at which the operator is found. Returns -1 if none is found
     */

    // Initial arbitrary values to compare against in the end
    int io_type = 42;
    int append_flag = 42;

    int i = 0;
    while (input[i] != NULL) {

        if (strcmp(input[i], ">") == 0) {
            io_type = STDOUT_FILENO;
            append_flag = 0;
            break;
        }
        else if (strcmp(input[i], ">>") == 0) {
            io_type = STDOUT_FILENO;
            append_flag = 1;
            break;
        }
        else if (strcmp(input[i], "<") == 0) {
            io_type = STDIN_FILENO;
            append_flag = 0;
            break;
        }
        i++;
    }

    // If both the initial values changed then an operator was found
    if (io_type != 42 && append_flag != 42) {
        if (input[i + 1] == NULL) {
            fprintf(stderr, "%serror%s: I/O redirection file unspecified\n", colors[ERR_COLOR], color_reset);
            exit(EXIT_FAILURE);
        }
        else {
            redirect_io(input[i + 1], io_type, append_flag);
            return i;
        }
    }

    return -1;
}

void sigchld_handler(int sig, siginfo_t *info, void *context) {
    /*
     * SIGCHLD handler that prints the pid of the background process that terminates
     */

    sigchld_flag = info->si_pid;

    // Disable signal handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = (void*) SIG_DFL;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGCHLD, &sa, NULL);
}
