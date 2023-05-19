#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "CODE/usage_time.h"

void upDateLimit(int);

int main(int argc, char* argv[]) {
    puts("*** this program should be run in background ***");

    bool execute = false;
    bool verbose = false;
    int interval;

    if (argc == 2 && is_number_string(argv[1])) {
        execute = true;
        interval = atoi(argv[1]);
    }
    if (argc == 3 && strcmp(argv[1], "-v") == 0 && is_number_string(argv[2])) {
        execute = true;
        verbose = true;
        interval = atoi(argv[2]);
    }

    if (!execute) {
        printf("Usage : %s [-v : verbose] <interval [sec]>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, cleanup_map);
    signal(SIGTERM, cleanup_map);

    setup_map();
    read_usage_time_from_file();

    while (1) {
        sleep(interval);
        exclude_process();
        write_usage_time_to_file();
        if (verbose) puts("usage time recorded...");
        upDateLimit(interval);
    }

    cleanup_map();
    return 0;
}

// update limit..
void upDateLimit(int interval) {
    FILE* file = fopen("left_time.log", "r");  // check time_limit
    FILE* temp_file = fopen("temp.log", "w");
    char line[256];
    char new_line[256];
    char buf[15];
    char* token;
    long long int limit = 0;

    while (fgets(line, sizeof(line), file) != NULL) {  // find the time

        token = strtok(line, ";");
        if (strlen(token) != 1 && token != NULL) {
            strcpy(new_line, token);
            token = strtok(NULL, ";");
            strcat(new_line, ";");
            strcat(new_line, token);

            token = strtok(NULL, ";");
            strcat(new_line, ";");

            limit = atoll(token);
            sprintf(buf, "%lld", limit - interval);
            strcat(new_line, buf);

            strcat(new_line, ";");
            token = strtok(NULL, ";");
            strcat(new_line, token);

            fputs(new_line, temp_file);
        }
    }

    rename("./temp.log", "./left_time.log");  // make new file

    fclose(file);
    fclose(temp_file);

    return;
}
