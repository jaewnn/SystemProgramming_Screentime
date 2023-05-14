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
void set_timeLimit();

char str[1024];
char blank[1024];

int main(int argc, char* argv[]) {
    // curses
    initscr();

    // usage time
    setup();

    int key;
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

        sleep(3);
        set_timeLimit();
    }

    // usage time
    cleanup();

    // curses
    endwin();
    return 0;
}

void print_legend() {
    memset(blank, ' ', COLS);
    sprintf(str, "%-5s %-32s %-16s %-16s ", "NO", "APP", "USAGE TIME", "TIME LEFTED");
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
        sprintf(str, "%-5d %-32s %02d:%02d:%02d ", i-6, name, (date->tm_mday - 1) * 24 + date->tm_hour - 9, date->tm_min, date->tm_sec);
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
    sprintf(str, "0) timeset\t1) search\t2) exit");
    strncpy(blank, str, strlen(str));
    move(LINES - 1, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void set_timeLimit()
{
    clear();
    refresh();

    move(LINES/2, COLS/2);
    standout();
    addnstr("Please Enter the NO of the process.", COLS);
    standend();
    refresh();

    int no;
    scanf("%d", &no); // 제한시간을 걸 번호를 선택

    clear();
    refresh();

    move(LINES/2, COLS/2);
    standout();
    addnstr("Please set the time limits (sec)", COLS);
    standend();
    refresh(); // time limit 설정

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
    time_t left_time; // 남은 시간
    struct tm* date;

    for (i = 7; i < LINES - 5; i++) {
        fscanf(fp, "%s %ld", name, &usage_time);
        date = localtime(&usage_time);

        if(i-6 == no) // 이름 추출
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

                if(strcmp(pid_name, name) == 0){ // 내가 찾는 놈이면 해당 pid 가져옴!
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
    if(readlink(file_path, pid_path, 200) == -1)
        sprintf(pid_path, "There is no absolute path..");


    // 남는 시간 계산
    left_time = limit - usage_time;

    /* 결과창 */
    clear();
    refresh();
    move(LINES/2-1, COLS/3);
    standout();
    addnstr("Time Limit is successfully saved!", COLS);
    move(LINES/2, COLS/3);
    sprintf(str, "Name : %s, Timelimit : %ld(sec), PID : %s, LeftTime: %ld(sec), path: %s\n", name, limit, pid, left_time, pid_path);
    addnstr(str, COLS);
    standend();
    refresh(); // time limit 설정


    /* 파일 쓰기 */
    sprintf(file_path, "left_time.log");
    FILE *fp2 = fopen(file_path, "a");
    sprintf(str, "\n%s;%s;%ld;%s", name, pid, left_time, pid_path);
    fputs(str, fp2);


    sleep(3);

    fclose(fp);
    fclose(fp2);
}
