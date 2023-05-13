/*
gcc -o demo_usage_time demo_usage_time.c CODE/usage_time.c hashmap.c/hashmap.c -lcurses
 */

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "CODE/usage_time.h"

void print_legend();
void print_data();
void print_menu();

char str[1024];
char blank[1024];

int main(int argc, char* argv[]) {
    // curses
    initscr();

    // usage time
    setup();

    while (1) {
        sleep(1);

        // usage time
        read_user_process_from_file();
        write_user_process_to_file();

        // curses
        print_legend();
        print_data();
        print_menu();
        refresh();
    }

    // usage time
    cleanup();

    // curses
    endwin();
    return 0;
}

void print_legend() {
    memset(blank, ' ', COLS);
    sprintf(str, " %-32s %-16s %-16s ", "APP", "USAGE TIME", "TIME LEFTED");
    strncpy(blank, str, strlen(str));
    move(6, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void print_data() {
    int i;
    time_t now = time(NULL);
    struct tm* week = localtime(&now);

    char file_path[32];
    sprintf(file_path, "usage_time_%d.log", week->tm_wday);

    FILE* fp = fopen(file_path, "r");
    char name[32];
    time_t usage_time;
    struct tm* date;

    for (i = 7; i < LINES - 5; i++) {
        fscanf(fp, "%s %ld", name, &usage_time);
        date = localtime(&usage_time);
        sprintf(str, " %-32s %02d:%02d:%02d ", name, (date->tm_mday - 1) * 24 + date->tm_hour - 9, date->tm_min, date->tm_sec);
        memset(blank, ' ', COLS);
        strncpy(blank, str, strlen(str));
        move(i, 0);
        addnstr(blank, COLS);
    }

    fclose(fp);

    memset(blank, '-', COLS);
    move(LINES - 5, 0);
    addnstr(blank, COLS);
    memset(blank, ' ', COLS);
    for (i = LINES - 4; i < LINES - 1; i++) {
        move(i, 0);
        addnstr(blank, COLS);
    }
    refresh();
}

void print_menu() {
    sprintf(str, "1) search\t2) exit");
    strncpy(blank, str, strlen(str));
    move(LINES - 1, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}
