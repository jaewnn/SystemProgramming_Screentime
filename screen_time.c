#include <curses.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "CODE/usage_time.h"

#define MAX_DATA_LINES 64
#define LEGEND_LINE_FROM_TOP 6
#define DATA_LINE_FROM_TOP 7
#define DATA_LINE_FROM_BOTTOM 7
#define INPUT_LINE_FROM_BOTTOM 6

#define NAME_BUF_SIZE 32

#define SET_TIME_LIMIT '1'
#define EXCLUDE_FROM_LIST '2'
#define EXIT '3'

void print_legend();
void print_data();
void print_menu();
void read_usage_time();

void select_process_by_number();
void set_timeLimit();
void exclude_from_list();

char str[1024];
char blank[1024];

name_time usage_time_arr[MAX_DATA_LINES];
int usage_time_count;

char name_buf[NAME_BUF_SIZE];

int main(int argc, char* argv[]) {
    initscr();
    cbreak();
    noecho();
    timeout(0);
    curs_set(0);  // diable cursor

    while (1) {
        int key = getch();

        if (key == SET_TIME_LIMIT) {
            set_timeLimit();
        } else if (key == EXCLUDE_FROM_LIST) {
            exclude_from_list();
        } else if (key == EXIT) {
            break;
        } else {
            read_usage_time();
            print_legend();
            print_data();
            print_menu();
        }
    }
    endwin();

    return 0;
}

void print_legend() {
    memset(blank, ' ', COLS);
    sprintf(str, "%-5s %-32s %-16s %-16s ", "NO", "APP", "USAGE TIME", "TIME LEFTED");
    strncpy(blank, str, strlen(str));
    move(LEGEND_LINE_FROM_TOP, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void print_data() {
    for (int i = DATA_LINE_FROM_TOP; i < LINES - DATA_LINE_FROM_BOTTOM; i++) {
        memset(blank, ' ', COLS);
        int data_index = i - DATA_LINE_FROM_TOP;

        if (data_index < usage_time_count) {
            struct tm* tm_ptr = localtime(&usage_time_arr[data_index].time);
            sprintf(str, "%-5d %-32s %02d:%02d:%02d ",
                    data_index + 1,
                    usage_time_arr[data_index].name,
                    (tm_ptr->tm_mday - 1) * 24 + tm_ptr->tm_hour - 9,
                    tm_ptr->tm_min,
                    tm_ptr->tm_sec);
            strncpy(blank, str, strlen(str));
        }

        move(i, 0);
        addnstr(blank, COLS);
    }

    memset(blank, '-', COLS);
    move(LINES - DATA_LINE_FROM_BOTTOM, 0);
    addnstr(blank, COLS);
    refresh();
}

void print_menu() {
    memset(blank, ' ', COLS);
    for (int i = LINES - INPUT_LINE_FROM_BOTTOM; i < LINES - 1; i++) {
        move(i, 0);
        addnstr(blank, COLS);
    }

    sprintf(str, "1) Set time limit    2) Exclude from list    3) Exit");
    strncpy(blank, str, strlen(str));
    move(LINES - 1, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void read_usage_time() {
    memset(usage_time_arr, 0, sizeof(name_time) * MAX_DATA_LINES);
    usage_time_count = 0;

    time_t now = time(NULL);
    struct tm* now_tm_ptr = localtime(&now);

    char filename[32];
    sprintf(filename, "usage_time_%d.log", now_tm_ptr->tm_wday);

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) return;

    while (!feof(fp)) {
        fscanf(fp, "%s %ld\n", usage_time_arr[usage_time_count].name, &usage_time_arr[usage_time_count].time);
        usage_time_count++;
    }

    fclose(fp);
}

void select_process_by_number() {
    nocbreak();   // enable canonical input mode
    echo();       // enable echo
    timeout(-1);  // blocking

    move(LINES - INPUT_LINE_FROM_BOTTOM, 0);
    addnstr("# of process: ", COLS);
    refresh();

    int number_of_process;
    int result = scanw("%d", &number_of_process);

    noecho();  // disable echo
    cbreak();  // disable canonical input mode

    if (result == -1 ||
        number_of_process - 1 < 0 ||
        number_of_process - 1 >= usage_time_count ||
        number_of_process >= LINES - DATA_LINE_FROM_TOP - DATA_LINE_FROM_BOTTOM + 1) {
        move(LINES - INPUT_LINE_FROM_BOTTOM + 1, 0);
        addnstr("Enter valid number. Press any key to continue...", COLS);
        getch();
    } else {
        memset(name_buf, 0, NAME_BUF_SIZE);
        strcpy(name_buf, usage_time_arr[number_of_process - 1].name);
    }

    timeout(0);  // non-blocking
}

void set_timeLimit() {

    echo();
    nocrmode();

    move(LINES-1, 0);
    standout();
    addnstr("Please Enter the NO of the process : ", COLS);
    standend();
    refresh();

    int no;
    scanf("%d", &no);  // 제한시간을 걸 번호를 선택

    move(LINES-1, 0);
    standout();
    addnstr("                                      ", COLS);
    move(LINES-1, 0);
    addnstr("Please set the time limits (sec) : ", COLS);
    standend();
    refresh();  // time limit 설정

    time_t limit;
    scanf("%ld", &limit);

    int i;
    time_t now = time(NULL);
    struct tm* week = localtime(&now);

    char file_path[100];
    sprintf(file_path, "usage_time_%d.log", week->tm_wday);

    FILE* fp = fopen(file_path, "r");
    char name[32];
    time_t usage_time;
    time_t left_time;  // 남은 시간
    struct tm* date;

    for (i = 7; i < LINES - 5; i++) {
        fscanf(fp, "%s %ld", name, &usage_time);
        date = localtime(&usage_time);

        if (i - 6 == no)  // 이름 추출
            break;
    }

    uid_t uid;
    DIR* dir_ptr;
    struct dirent* dirent_ptr;
    struct stat statbuf;
    char pid_name[50];
    char pid[30];

    uid = geteuid();

    if ((dir_ptr = opendir("/proc")) == NULL) {
        perror("opendir");
        exit(1);
    }

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (is_number_string(dirent_ptr->d_name)) {
            if (is_user_process(&statbuf, dirent_ptr->d_name, uid)) {
                // pid로 이름 받아오기
                get_process_name_by_pid_string(pid_name, dirent_ptr->d_name);

                if (strcmp(pid_name, name) == 0) {  // 내가 찾는 놈이면 해당 pid 가져옴!
                    strcpy(pid, dirent_ptr->d_name);
                    break;
                }
                memset(pid_name, 0, 30);
            }
        }
    }

    /* 경로 읽어오기 */
    sprintf(file_path, "/proc/%s/exe", pid);
    char pid_path[200];
    if (readlink(file_path, pid_path, 200) == -1)
        sprintf(pid_path, "There is no absolute path..");

    // 남는 시간 계산
    left_time = limit - usage_time;

    /* 결과창 */
    move(LINES - 2, 0);
    standout();
    addnstr("Time Limit is successfully saved!", COLS);
    standend();

    move(LINES-1, 0);
    standout();
    sprintf(str, "Name : %s, Timelimit : %ld(sec), PID : %s, LeftTime: %ld(sec), path: %s\n", name, limit, pid, left_time, pid_path);
    addnstr(str, COLS);
    standend();
    refresh();  // time limit 설정

    /* 파일 쓰기 */
    sprintf(file_path, "left_time.log");
    FILE* fp2 = fopen(file_path, "a");
    sprintf(str, "\n%s;%s;%ld;%s", name, pid, left_time, pid_path);
    fputs(str, fp2);

    // sleep(5)

    fclose(fp);
    fclose(fp2);
    noecho();
    crmode();
}
