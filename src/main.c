#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "execute.h"
#include "parse.h"

#define MAX_SIZE 256
#define MAX_PROMPT_SIZE  16384
#define FILLER_LINE_SIZE 8129

// ==================================== globals ==================================== 

// Flag set by the SIGCHLD signal handler. contains the terminated child pid
int sigchld_flag = 0;

// Global constants for the username and hostname
char* username;
char* hostname;

FILE* logfile;
const char* logfile_name = "cash.log";

// ANSI color codes     0: Red      1: Green    2: Yellow   3: Blue     4: Purple   5: Cyan
const char* colors[] = {"\e[0;31m", "\e[0;32m", "\e[0;33m", "\e[0;34m", "\e[0;35m", "\e[0;36m"};
const char* color_reset = "\e[0m";

int accent_color = 5;

// ================================================================================= 

int main() {

    print_greeting();

    // Arguments vector
    char** input;

    char*** array_of_inputs;

    // Initialize username and hostname
    username = getlogin();
    hostname = malloc(sizeof(char) * MAX_SIZE);
    gethostname(hostname, MAX_SIZE);
    hostname = realloc(hostname, sizeof(char) * (strlen(hostname) + 1));

    // Clear log file
    logfile = fopen(logfile_name, "w");
    fclose(logfile);

    while (1) {
        input = parse_input();

        array_of_inputs = separate_inputs(input);

        if (array_of_inputs == NULL)
            execute_input(input);
        else
            execute_piped_inputs(array_of_inputs);


        free_array_of_inputs(array_of_inputs);
        free_input(input);
    }

    return 0;
}

void free_input(char** input) {
    /*
     * Frees each malloc'd string and the malloc'd array of char pointers
     */

    if (input == NULL)
        return;

    int i = 0;
    while(input[i] != NULL) {
        free(input[i]);
        i++;
    }

    free(input);
}

void free_array_of_inputs(char*** array_of_inputs) {
    /*
     */

    if (array_of_inputs == NULL)
        return;

    int i = 0;
    while(array_of_inputs[i] != NULL) {
        free_input(array_of_inputs[i]);
        i++;
    }

    free(array_of_inputs);
}

char* generate_prompt() {
    /*
     * Generates a cute looking prompt
     *
     * Returns: The generated prompt
     */

    // get terminal size
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    // get current working directory
    char current_dir[MAX_SIZE];
    getcwd(current_dir, sizeof(current_dir));

    if (strcmp(current_dir, getenv("HOME")) == 0)
        strcpy(current_dir, "~");

    // truncate the last two directories by default
    truncate_dir(current_dir, 2);

    // note: 7 is the other decorative characters
    int filler_line_length = w.ws_col - (strlen(username) + strlen(hostname) + strlen(current_dir) + 7);

    // fill out the filler line
    char filler_line[FILLER_LINE_SIZE];
    filler_line[0] = '\0';
    
    for (int i = 0; i < filler_line_length; i++)
        strcat(filler_line, "─");

    char* prompt = malloc(sizeof(char) * MAX_PROMPT_SIZE);
    snprintf(prompt, MAX_PROMPT_SIZE, "┌─{%s%s%s@%s%s%s}%s{%s%s%s}\n└─%s♥%s ",
            colors[accent_color], username, color_reset,
            colors[accent_color], hostname, color_reset, filler_line,
            colors[accent_color], current_dir, color_reset,
            colors[accent_color], color_reset);

    prompt = realloc(prompt, sizeof(char) * (strlen(prompt) + 1));

    return prompt;
}

void truncate_dir(char* dir_name, int truncate_length) {
    /*
     * Truncates the working directory string to the last specified directories
     *
     * Arguments:
     *  dir_name: A string containing the directory name
     *  truncate_length: The number of remaining directories in the end after truncating the rest
     */

    int slash_occurences = 0;
    
    // First count the number of occurences of the forward slash character
    int i = 0;
    while(dir_name[i] != '\0') {
        if (dir_name[i] == '/')
            slash_occurences++;

        i++;
    }

    if (slash_occurences >= truncate_length) {
        slash_occurences = 0;

        // Then we'll start from the end of the string, looking for the point at which to truncate
        int j = strlen(dir_name) - 1;
        while (slash_occurences != truncate_length) {
            if (dir_name[j] == '/')
                slash_occurences++;

            j--;
        }

        // Readjust the counter
        j += 2;

        // Overwrite the truncated string on top of the original string
        memcpy(dir_name, &dir_name[j], i - j + 1);
    }
}

void print_greeting() {
    /*
     * Clears the terminal, then prints an ascii "logo" and a greeting text
     */

    // Get terminal size
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    // Space filled strings to center the logo and text
    char logo_filler_space[FILLER_LINE_SIZE];
    char text_filler_space[FILLER_LINE_SIZE];

    // Widths of the logo and the text to calculate how much space is needed to center
    int logo_width = 46;
    int text1_width = strlen("Welcome to CASH! The cute awesome shell.");

    int i;
    for (i = 0; i < (w.ws_col - logo_width)/2; i++)
        logo_filler_space[i] = ' ';
    logo_filler_space[i] = '\0';

    int j;
    for (j = 0; j < (w.ws_col - text1_width)/2; j++)
        text_filler_space[j] = ' ';
    text_filler_space[j] = '\0';


    printf("\033[2J\033[1;1H");
    printf("%s╔════════════════════════════════════════════╗\n", logo_filler_space);
    printf("%s║                                            ║\n", logo_filler_space);
    printf("%s║%s    ██████      ██      ████████ ██      ██%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s   ██░░░░██    ████    ██░░░░░░ ░██     ░██%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s  ██    ░░    ██░░██  ░██       ░██     ░██%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s ░██         ██  ░░██ ░█████████░██████████%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s ░██        ██████████░░░░░░░░██░██░░░░░░██%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s ░░██    ██░██░░░░░░██       ░██░██     ░██%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s  ░░██████ ░██     ░██ ████████ ░██     ░██%s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║%s   ░░░░░░  ░░      ░░ ░░░░░░░░  ░░      ░░ %s ║\n", logo_filler_space, colors[accent_color], color_reset);
    printf("%s║                                            ║\n", logo_filler_space);
    printf("%s╚════════════════════════════════════════════╝\n", logo_filler_space);
    printf("\n%sWelcome to %sCASH%s! The cute awesome shell.\n", text_filler_space, colors[accent_color], color_reset);
    printf("%sType %shelp%s to see the available features.\n\n", text_filler_space, colors[accent_color], color_reset);
}
