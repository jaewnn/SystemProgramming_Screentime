#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "usage_time.h"

int main(int argc, char *argv[]) {
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
        process_message();
        write_usage_time_to_file();
        update_lefted(interval);

        if (verbose)
            puts("usage time recorded...");
    }

    cleanup_map();
    return 0;
}
