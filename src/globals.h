#ifndef GLOBALS_H
#define GLOBALS_H

#include "main.h"

#define RED     0
#define GREEN   1
#define YELLOW  2
#define BLUE    3
#define PURPLE  4
#define CYAN    5

#define PID_COLOR BLUE
#define ERR_COLOR RED

extern BgProcess bg_processes[];

// Flag set by the SIGCHLD signal handler. contains the terminated child pid
extern int sigchld_flag;

// Global constants for the username and hostname
extern char* username;
extern char* hostname;

// ANSI color codes
extern const char* colors[];
extern const char* color_reset;

extern int accent_color;

#endif
