#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "parse.h"

#include "globals.h"

#define MAX_SIZE 256

char** parse_input() {
    /*
     * Parses a user input string via the readline library
     *
     * Returns: A null termintaed array of char pointers (strings)
     */

    if (sigchld_flag != -1) {
        // printf("[%d] done\n", sigchld_flag);
        remove_bg_process(sigchld_flag);
        sigchld_flag = -1;
    }

    // Generate prompt and start readline input
    char* prompt = generate_prompt();
    char* input_buffer = readline(prompt);

    int quotes_occurrences = char_occurrences(input_buffer, '"');

    if (quotes_occurrences % 2 == 1) {
        fprintf(stderr, "%serror%s: mismatched number of string quotes\n", colors[ERR_COLOR], color_reset);
        return NULL;
    }

    // Add history if line is not empty
    if (input_buffer && *input_buffer)
        add_history(input_buffer);

    // Making an argv for execvp, which is an array of strings containing the arguments for the program
    int arguments_number = 0;
    char** program_arguments = malloc(sizeof(char*) * MAX_SIZE);

    // Split the input buffer string into space-separated substrings
    char* word = strtok(input_buffer, " ");

    // Go over each substring
    while(word != NULL) {
        // Treat the word normally if either it doesn't contain a quote
        // or if our input in general doesn't contain any quotes
        if (quotes_occurrences == 0 || strchr(word, '"') == NULL) {
            program_arguments[arguments_number] = malloc(sizeof(char) * (strlen(word) + 1));
            strcpy(program_arguments[arguments_number], word);

            arguments_number++;
            word = strtok(NULL, " ");
        }
        else {
            // If a single word has more than 2 quotes something's wrong
            if (char_occurrences(word, '"') > 2) {
                fprintf(stderr, "%serror%s: misuse of string quotes\n", colors[ERR_COLOR], color_reset);
                return NULL;
            }
            else if (char_occurrences(word, '"') == 2) {
                // If the word has exactly 2 quotes placed properly around it, then we take the word itself
                if (word[0] == '"' && word[strlen(word) - 1] == '"') {
                    program_arguments[arguments_number] = malloc(sizeof(char) * (strlen(del_char(word, '"')) + 1));
                    strcpy(program_arguments[arguments_number], del_char(word, '"'));

                    quotes_occurrences -= 2;
                    arguments_number++;
                    word = strtok(NULL, " ");
                }
                else {
                    fprintf(stderr, "%serror%s: misuse of string quotes\n", colors[ERR_COLOR], color_reset);
                    return NULL;
                }
            }
            else {
                // Initialize an empty string with enough space
                program_arguments[arguments_number] = malloc(sizeof(char) * MAX_SIZE * 8);
                program_arguments[arguments_number][0] = '\0';

                // Loop until we have no more quotes to process
                while (quotes_occurrences != 0) {

                    // Handle if a quote is in the middle of the word
                    if (strchr(word, '"') != NULL) {
                        if (word[0] != '"' && word[strlen(word) - 1] != '"') {
                            fprintf(stderr, "%serror%s: misuse of string quotes\n", colors[ERR_COLOR], color_reset);
                            return NULL;
                        }
                    }

                    strcat(program_arguments[arguments_number], del_char(word, '"'));
                    strcat(program_arguments[arguments_number], " ");

                    if (strchr(word, '"') != NULL) {
                        quotes_occurrences--;
                    }

                    word = strtok(NULL, " ");
                }

                program_arguments[arguments_number] = realloc(program_arguments[arguments_number], sizeof(char) * (strlen(program_arguments[arguments_number]) + 1));

                // Remove the trailing space
                int last_space_index = strlen(program_arguments[arguments_number]) - 1;
                program_arguments[arguments_number][last_space_index] = '\0';

                arguments_number++;
            }
        }
    }

    // The arguments array must be null terminated
    program_arguments[arguments_number] = NULL;

    program_arguments = realloc(program_arguments, sizeof(char*) * (arguments_number + 1));

    free(prompt);
    free(input_buffer);

    return program_arguments;
}

char*** separate_inputs(char** input) {
    /*
     * Takes a parsed input array and separates it into an array of inputs according to a delimiter
     *
     * Arguments:
     *  input: A null terminated array of char pointers (strings)
     *
     * Returns: A NULL terminated array of NULL terminated input arrrays
     */

    if (input == NULL)
        return NULL;

    char* delim = "|";
    int delim_occurence = 0;

    // Count the number of delimiter occurences
    int i = 0;
    while(input[i] != NULL) {
        if (strcmp(input[i], delim) == 0)
            delim_occurence++;

        i++;
    }

    if (delim_occurence == 0)
        return NULL;

    // The number of separate inputs will always be one more than the number of delimiters,,
    // we add another one because the array is null terminated
    char*** array_of_inputs = malloc(sizeof(char**) * (delim_occurence + 2));

    // Initialize each input argv
    for (int j = 0; j < delim_occurence + 1; j++)
        array_of_inputs[j] = malloc(sizeof(char**) * MAX_SIZE);

    int input_counter = 0;
    int pipe_array_counter = 0;
    int word_counter = 0;

    // Go over the original input, copying each word to its corresponding new input argv in array_of_inputs
    while(input[input_counter] != NULL) {
        if (strcmp(input[input_counter], delim) != 0) {
            array_of_inputs[pipe_array_counter][word_counter] = malloc(sizeof(char) * (strlen(input[input_counter]) + 1));
            strcpy(array_of_inputs[pipe_array_counter][word_counter], input[input_counter]);

            word_counter++;
        }
        else {
            // Null terminate each argv
            array_of_inputs[pipe_array_counter][word_counter] = NULL;
            array_of_inputs[pipe_array_counter] = realloc(array_of_inputs[pipe_array_counter], sizeof(char**) * (word_counter + 1));

            pipe_array_counter++;

            if (input[input_counter + 1] != NULL)
                word_counter = 0;
        }

        input_counter++;
    }

    // Null terminate the last argv
    array_of_inputs[pipe_array_counter][word_counter] = NULL;

    // Null terminate the array of argvs itself
    array_of_inputs[delim_occurence + 1] = NULL;

    return array_of_inputs;
}

char* del_char(char *str, char garbage_char) {
    /*
     * Deletes a character from a given string
     *
     * Arguments:
     *  str: The given string
     *  garbage_char: The character to remove
     *
     * Returns: The string after the character is deleted.
     *          If the character is not found it returns the same string
     */

    char* new_str = malloc(sizeof(char) * strlen(str));

    int i = 0, j = 0;
    while (str[i] != '\0') {
        if (str[i] != garbage_char) {
            new_str[j] = str[i];
            i++;
            j++;
        }
        else {
            i++;
        }
    }

    new_str[j] = '\0';

    return new_str;
}

int char_occurrences(char *str, char chr) {
    /*
     * Count the number of occurrences in a strig
     *
     * Arguments:
     *  str: The given string
     *  chr: The character to count its occurrences
     *
     * Returns: The number of occurrences
     */

    int occurrences = 0;

    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '"')
            occurrences++;

        i++;
    }

    return occurrences;
}

void add_bg_process(pid_t cpid, char** argv) {
    /*
     * Adds a process to the array of currently running background processes
     *
     * Arguments:
     *  cpid: The pid of the child process
     *  argv: The NULL terminated input array of strings
     */

    for (int i = 0; i < MAX_BG_PROC; i++) {
        if (bg_processes[i].pid == -1) {
            bg_processes[i].pid = cpid;

            printf("[%s%d%s] started in the background\n", colors[PID_COLOR], cpid, color_reset);

            bg_processes[i].command = malloc(sizeof(char*) * MAX_SIZE);

            int j = 0;
            while (argv[j] != NULL) {
                bg_processes[i].command[j] = malloc(sizeof(char) * (strlen(argv[j]) + 1));
                strcpy(bg_processes[i].command[j], argv[j]);
                j++;
            }

            bg_processes[i].command[j] = NULL;

            bg_processes[i].command = realloc(bg_processes[i].command, sizeof(char*) * (j + 1));

            break;
        }
    }
}

void remove_bg_process(pid_t cpid) {
    /*
     * Removes a process from the array of currently running background processes
     *
     * Arguments:
     *  cpid: The pid of the child process
     */

    for (int i = 0; i < MAX_BG_PROC; i++) {
        if (bg_processes[i].pid == cpid) {
            bg_processes[i].pid = -1;

            printf("[%s%d%s] done\n", colors[PID_COLOR], cpid, color_reset);

            int j = 0;
            while (bg_processes[i].command[j] != NULL) {
                printf("%s ", bg_processes[i].command[j]);
                j++;
            }

            printf("\n");

            break;
        }
    }
}
