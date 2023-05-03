#include <curses.h>
#include <stdio.h>
#include <string.h>

void print_legend();
void print_data();
void print_menu();

char str[1024];
char blank[1024];

int main(int argc, char* argv[]) {
    initscr();
    crmode();
    noecho();
    print_legend();
    print_data();
    print_menu();
    refresh();

    int key;
    while (1) {
        key = getch();
        if (key == KEY_RESIZE) {
            print_legend();
            print_data();
            print_menu();
        }
        if (key == '2') {
            break;
        }
    }

    endwin();
    return 0;
}

void print_legend() {
    memset(blank, ' ', COLS);
    sprintf(str, " %6s %-8s %3s %3s %7s %6s %6s %1s %5s %5s %9s %s", "PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "%CPU", "%MEM", "TIME+", "COMMAND");
    strncpy(blank, str, strlen(str));
    move(6, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void print_data() {
    int i;
    memset(blank, ' ', COLS);
    for (i = 7; i < LINES - 5; i++) {
        sprintf(str, " %6d %-8s %3s %3s %7s %6s %6s %1s %5s %5s %9s %s", i, "test", "tes", "tes", "test", "test", "test", "S", "test", "test", "test", "test");
        strncpy(blank, str, strlen(str));
        move(i, 0);
        addnstr(blank, COLS);
    }
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
