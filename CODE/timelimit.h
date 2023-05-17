#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_TOKENS 4
#define MAX_TOKEN_LENGTH 100
#define MAX_LINE_LENGTH 100

void splitString(char* str,char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH],int* numTokens);
void execute_remove(char* program_path);
void execute_recover();
void time_limit();
