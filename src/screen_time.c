#include <curses.h>

#include "screen_time_func.h"

int main(int argc, char *argv[]) {
    initscr();
    cbreak();
    noecho();
    timeout(0);
    curs_set(0); // diable cursor

    read_prev_usage_time();

    while (1) {
        int key = getch();

        if (key == SET_TIME_LIMITS) {
            set_time_limits();
        } else if (key == EXCLUDE_FROM_LIST) {
            exclude_from_list();
        } else if (key == EXIT) {
            break;
        } else {
            read_usage_time();
            print_access_time();
            print_graph();
            print_legend();
            print_data();
            print_menu();
        }
    }
    endwin();

    return 0;
}
