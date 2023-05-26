#include <curses.h>
#include <dirent.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "CODE/timelimit.h"
#include "CODE/usage_time.h"

#define MAX_DATA_LINES 64
#define ACCESS_TIME_LINE_FROM_TOP 0
#define GRAPH_LINE_FROM_TOP 2
#define LEGEND_LINE_FROM_TOP 10
#define DATA_LINE_FROM_TOP 11
#define DATA_LINE_FROM_BOTTOM 7
#define INPUT_LINE_FROM_BOTTOM 6

#define NAME_BUF_SIZE 32

#define SET_TIME_LIMIT '1'
#define EXCLUDE_FROM_LIST '2'
#define EXIT '3'

#define WEEK 7

int timeLimitSIG = 0;

typedef unsigned long long int LLI;
typedef struct proc* procPointer;
typedef struct proc {
    unsigned long pid;   // 현재 실행중인 process id
    unsigned long uid;   // 유저 id
    char user_name[30];  // 유저 이름
    LLI priority;        // PI (우선순위)
    LLI nice;            // NI (NICE 값)
    LLI virt;            // VIRT (KB기준)
    LLI res;             // RES
    LLI shr;             // SHR
    char state;          // 프로세스 상태
    LLI utime;
    LLI stime;
    LLI start_time;  // cpu사용률 계산용
    float cpu;       // cpu 사용률
    float mem;       // 메모리 사용률
    float time;      // CPU 사용시간 (0.01초 단위)
    LLI convert_time;
    char command[30];  // 프로세스 사용시 명령어

    procPointer next;  // 연결 포인터
} proc;

void get_info_from_name(procPointer*, char*);
procPointer make_proc();
procPointer add_process(char*);

void get_stat_file(procPointer*, char*);
void get_status_file(procPointer*, char*);
void get_cpu_use(procPointer*);
void get_mem_use(procPointer*);

void read_prev_usage_time();
void read_usage_time();
void print_access_time();
void print_graph();
void print_legend();
void print_data();
void print_menu();

void select_process_by_number();
void set_timeLimit();
void exclude_from_list();

char str[1024];
char blank[1024];

name_time usage_time_arr[MAX_DATA_LINES];
int usage_time_count;

char name_buf[NAME_BUF_SIZE];

time_t prev_usage_time[WEEK];

int main(int argc, char* argv[]) {
    // 실행권한 복구 하는 옵션
    if ((argc == 2) && (strcmp(argv[1], "-r") == 0)) {
        printf("Recover execute permision...\n");
        execute_recover();
        return 0;
    }

    initscr();
    cbreak();
    noecho();
    timeout(0);
    curs_set(0);  // diable cursor

    read_prev_usage_time();

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
            print_access_time();
            print_graph();
            print_legend();
            print_data();
            print_menu();
            //            if(timeLimitSIG == 1){
            //              time_limit();
            //            }
        }
    }
    endwin();

    return 0;
}

procPointer make_proc() {
    procPointer proc_node;
    proc_node = (procPointer)malloc(sizeof(proc));
    proc_node->pid = 0;  // 현재 실행중인 process id
    proc_node->uid;
    proc_node->priority = 0;  // PI (우선순위)
    proc_node->nice = 0;      // NI (NICE 값)
    proc_node->virt = 0;      // VIRT (KB기준)
    proc_node->res = 0;       // RES
    proc_node->shr = 0;       // SHR
    proc_node->state = 'F';   // 프로세스 상태

    proc_node->cpu = 0;  // cpu 사용률
    proc_node->mem = 0;  // 메모리 사용률

    proc_node->next = NULL;

    return proc_node;
}

void print_access_time() {
    FILE* fp = fopen("access_time.log", "r");
    if (fp == NULL)
        sprintf(str, "Access time: None");
    else {
        time_t access_time;
        fscanf(fp, "%ld", &access_time);
        sprintf(str, "Access time: %s", ctime(&access_time));
        fclose(fp);
    }

    move(ACCESS_TIME_LINE_FROM_TOP, 0);
    addnstr(str, COLS);
    refresh();
}

void print_graph() {
    time_t now = time(NULL);
    struct tm* now_tm_ptr = localtime(&now);

    for (int i = 0; i < WEEK; i++) {
        int weekday = (now_tm_ptr->tm_wday + i + 1) % WEEK;
        int width = (prev_usage_time[weekday] * (COLS - 4)) / (24 * 60 * 60);

        char graph[COLS];
        memset(graph, ' ', COLS);
        memset(graph, '|', width < COLS ? width : COLS);

        if (weekday == 0)
            sprintf(str, "SUN %s", graph);
        if (weekday == 1)
            sprintf(str, "MON %s", graph);
        if (weekday == 2)
            sprintf(str, "TUE %s", graph);
        if (weekday == 3)
            sprintf(str, "WED %s", graph);
        if (weekday == 4)
            sprintf(str, "THU %s", graph);
        if (weekday == 5)
            sprintf(str, "FRI %s", graph);
        if (weekday == 6)
            sprintf(str, "SAT %s", graph);

        move(GRAPH_LINE_FROM_TOP + i, 0);
        addnstr(str, COLS);
    }
    refresh();
}

void print_legend() {
    memset(blank, ' ', COLS);
    sprintf(str, "%-5s %6s %-32s %3s %3s %7s %6s %6s %3s %4s %4s    %10s  %10s ",
            "NO", "PID", "APP", "PR", "NI", "VIRT", "RES", "SHR", "S", "\%CPU", "\%MEM", "USAGE TIME", "TIME LEFTED");
    strncpy(blank, str, strlen(str));
    move(LEGEND_LINE_FROM_TOP, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void print_data() {
    char file_limit_path[32];
    sprintf(file_limit_path, "left_time.log");
    FILE* fp_limit = fopen(file_limit_path, "r");  // check time_limit

    char line[256];
    char* token;

    for (int i = DATA_LINE_FROM_TOP; i < LINES - DATA_LINE_FROM_BOTTOM; i++) {
        memset(blank, ' ', COLS);
        int data_index = i - DATA_LINE_FROM_TOP;

        if (data_index < usage_time_count) {
            // show time limit time
            int limit = 0;
            while (fgets(line, sizeof(line), fp_limit) != NULL) {  // find the limit
                token = strtok(line, ";");
                if (strlen(token) != 1 && token != NULL && (strcmp(token, usage_time_arr[data_index].name) == 0)) {
                    token = strtok(NULL, ";");
                    token = strtok(NULL, ";");
                    limit = atoi(token);
                    break;
                }
            }

            fseek(fp_limit, 0, SEEK_SET);

            int hours = limit / 3600;
            int minutes = (limit % 3600) / 60;
            int seconds = limit % 60;

            procPointer info = make_proc();  // bring information about program
            get_info_from_name(&info, usage_time_arr[data_index].name);

            struct tm* tm_ptr = localtime(&usage_time_arr[data_index].time);
            sprintf(str, "%-5d %6ld %-32s %3lld %3lld %7lld %6lld %6lld %3c %4.1f %4.1f      %02d:%02d:%02d     %02d:%02d:%02d",
                    data_index + 1,
                    info->pid,
                    usage_time_arr[data_index].name,
                    info->priority,
                    info->nice,
                    info->virt,
                    info->res,
                    info->shr,
                    info->state,
                    info->cpu,
                    info->mem,
                    (tm_ptr->tm_mday - 1) * 24 + tm_ptr->tm_hour - 9,
                    tm_ptr->tm_min,
                    tm_ptr->tm_sec,
                    hours,
                    minutes,
                    seconds);
            strncpy(blank, str, strlen(str));
            free(info);
        }

        move(i, 0);
        addnstr(blank, COLS);
    }

    fclose(fp_limit);

    memset(blank, '-', COLS);
    move(LINES - DATA_LINE_FROM_BOTTOM, 0);
    addnstr(blank, COLS);
    refresh();
}

void get_info_from_name(procPointer* info, char* name) {
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
            get_process_name_by_pid_string(pid_name, dirent_ptr->d_name);

            if (strcmp(pid_name, name) == 0) {  // 내가 찾는 놈이면 해당 pid 가져옴!
                strcpy(pid, dirent_ptr->d_name);
                break;
            }
            memset(pid_name, 0, 30);
        }
    }

    if (atoll(pid) != 0)
        (*info) = add_process(pid);

    closedir(dir_ptr);

    // *info =  add_process(pid);
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

void read_prev_usage_time() {
    memset(prev_usage_time, 0, sizeof(time_t) * WEEK);

    time_t now = time(NULL);
    struct tm* now_tm_ptr = localtime(&now);

    for (int i = 0; i < WEEK; i++) {
        if (i == now_tm_ptr->tm_wday) continue;

        char filename[32];
        sprintf(filename, "usage_time_%d.log", i);

        FILE* fp = fopen(filename, "r");
        if (fp == NULL) continue;

        char name_buf[NAME_BUF_SIZE];
        time_t time_buf;
        while (!feof(fp)) {
            fscanf(fp, "%s %ld\n", name_buf, &time_buf);
            prev_usage_time[i] += time_buf;
        }

        fclose(fp);
    }
}

void read_usage_time() {
    memset(usage_time_arr, 0, sizeof(name_time) * MAX_DATA_LINES);
    usage_time_count = 0;

    time_t now = time(NULL);
    struct tm* now_tm_ptr = localtime(&now);

    char filename[32];
    sprintf(filename, "usage_time_%d.log", now_tm_ptr->tm_wday);

    prev_usage_time[now_tm_ptr->tm_wday] = 0;

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) return;

    char name_buf[NAME_BUF_SIZE];
    time_t time_buf;
    while (!feof(fp)) {
        if (usage_time_count < MAX_DATA_LINES) {
            fscanf(fp, "%s %ld\n", usage_time_arr[usage_time_count].name, &usage_time_arr[usage_time_count].time);
            prev_usage_time[now_tm_ptr->tm_wday] += usage_time_arr[usage_time_count].time;
            usage_time_count++;
        } else {
            fscanf(fp, "%s %ld\n", name_buf, &time_buf);
            prev_usage_time[now_tm_ptr->tm_wday] += time_buf;
            usage_time_count++;
        }
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

    move(LINES - 1, 0);
    standout();
    addnstr("Please Enter the NO of the process :                   ", COLS);
    standend();
    refresh();

    int no;
    scanf("%d", &no);  // 제한시간을 걸 번호를 선택

    move(LINES - 1, 0);
    standout();
    addnstr("                                      ", COLS);
    move(LINES - 1, 0);
    addnstr("Please set the time limits (sec) :                     ", COLS);
    standend();
    refresh();  // time limit 설정

    int limit;
    scanf("%d", &limit);

    int i;
    time_t now = time(NULL);
    struct tm* week = localtime(&now);

    char file_path[100];
    sprintf(file_path, "usage_time_%d.log", week->tm_wday);

    FILE* fp = fopen(file_path, "r");
    char name[32];
    time_t usage_time;
    int left_time;  // 남은 시간
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
                memset(pid_name, 0, 50);
            }
        }
    }

    /* 경로 읽어오기 */
    sprintf(file_path, "/proc/%s/exe", pid);
    char pid_path[100];
    memset(pid_path, 0, 100);
    if (readlink(file_path, pid_path, 100) == -1)
        sprintf(pid_path, "There is no absolute path..");

    // 남는 시간 계산
    left_time = limit;

    /* 결과창 */
    move(LINES - 2, 0);
    standout();
    addnstr("Time Limit is successfully saved!", COLS);
    standend();

    move(LINES - 1, 0);
    standout();
    sprintf(str, "Name : %s, Timelimit : %d(sec), PID : %s, LeftTime: %d(sec), path: %s\n", name, limit, pid, left_time, pid_path);
    addnstr(str, COLS);
    standend();
    timeLimitSIG = 1;
    refresh();  // time limit 설정

    /* 파일 쓰기 */
    sprintf(file_path, "left_time.log");
    FILE* fp2 = fopen(file_path, "a");
    sprintf(str, "\n%s;%s;%d;%s", name, pid, left_time, pid_path);
    fputs(str, fp2);

    // sleep(5)

    fclose(fp);
    fclose(fp2);
    noecho();
    crmode();
}

void exclude_from_list() {
    select_process_by_number();

    struct mq_attr attr = {.mq_maxmsg = 4, .mq_msgsize = NAME_BUF_SIZE};
    mqd_t mq = mq_open("/exclude_mq", O_WRONLY | O_CREAT, 0666, &attr);

    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    int result = mq_send(mq, name_buf, NAME_BUF_SIZE, 0);
    if (result == -1) {
        perror("mq_send");
        mq_close(mq);
        exit(1);
    }

    mq_close(mq);
}

/* 현재 실행중인 프로세스 정보 노드를 생성하는 함수 */
procPointer add_process(char* my_pid) {
    struct stat statbuf;
    struct passwd* passbuf;                              // 둘 모두 UserID를 가져오기 위함
    char* proc_path = (char*)malloc(sizeof(char) * 30);  // progress 정보 얻는 경로

    procPointer new_process = make_proc();
    new_process->next = NULL;
    unsigned long pid = atoll(my_pid);
    new_process->pid = pid;  // process id

    sprintf(proc_path, "/proc/%s/stat", my_pid);
    stat(proc_path, &statbuf);  // get stat
    new_process->uid = statbuf.st_uid;
    passbuf = getpwuid(new_process->uid);
    strcpy(new_process->user_name, passbuf->pw_name);  // get User name

    /* Get all information from stat file : PR, NI, state */
    get_stat_file(&new_process, proc_path);

    /* Get all information from status file : VIRT, RES, SHR, Command */
    sprintf(proc_path, "/proc/%s/status", my_pid);
    get_status_file(&new_process, proc_path);

    /* Bring Cpu and Memory using rate */
    get_cpu_use(&new_process);
    get_mem_use(&new_process);

    free(proc_path);

    return new_process;
}

/* proc/PID/stat file에서 정보를 읽어옴 */
void get_stat_file(procPointer* proc_info, char* path) {
    LLI spid, ppid, sid, pgid, tty_nr, tty_pgrp, flags;  // 나의 PID 부모 PID, 그룹 ID, process session ID, control하는 terminal, (모름), 리눅스 플래그
    LLI min_flt, cmin_flt, maj_flt, cmaj_flt, utime, stimev;
    LLI cutime, cstime, priority, nice, num_threads, it_real_value;
    LLI vsize, rss;                                  // 메모리 사이즈, (모름)
    unsigned long long start_time;                   // 프로세스 시작 시간
    char state;                                      // R, S, D, Z, T (S열 : 상태)
    char* comm = (char*)malloc(sizeof(char) * 256);  // 실행 가능한 파일 이름

    /* process 정보가 담긴 stat 파일 열기 */
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Failed to open /proc/[pid]/stat file");
        exit(-1);
    }

    // process의 자세한 정보들을 읽어온다.
    fscanf(fp, "%lld %s %c %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %llu %lld %lld",
           &spid, comm, &state, &ppid, &pgid, &sid, &tty_nr, &tty_pgrp, &flags, &min_flt, &cmin_flt, &maj_flt, &cmaj_flt, &utime, &stimev,
           &cutime, &cstime, &priority, &nice, &num_threads, &it_real_value, &start_time, &vsize, &rss);

    (*proc_info)->priority = priority;      // PR
    (*proc_info)->nice = nice;              // NI
    (*proc_info)->state = state;            // State
    (*proc_info)->utime = utime;            // utime
    (*proc_info)->stime = stimev;           // stime
    (*proc_info)->start_time = start_time;  // start time

    free(comm);
    fclose(fp);
}

/* proc/PID/status file에서 정보를 읽어옴 */
void get_status_file(procPointer* proc_info, char* path) {
    /* process 정보가 담긴 status 파일 열기 */
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Failed to open /proc/[pid]/status file");
        exit(-1);
    }

    char* status_line = (char*)malloc(sizeof(char) * 500);
    char* name = (char*)malloc(sizeof(char) * 50);  // key를 읽어냄

    (*proc_info)->virt = 0;  // VIRT : 가상메모리 사용량
    (*proc_info)->res = 0;   // RES  : 물리메모리 사용량
    (*proc_info)->shr = 0;   // SHR  : 공유메모리 사용량
    /* status 파일 한 줄씩 읽기 */
    while (fp != NULL && feof(fp) == 0) {
        fgets(status_line, sizeof(char) * 500, fp);
        name = strtok(status_line, ":");

        if (strcmp(name, "VmSize") == 0)  // status 파일에 VmSize (VIRT 값 있으면 받아옴)
        {
            name = strtok(NULL, "k");
            (*proc_info)->virt = atoll(name);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET);  // 처음 위치로 돌림
    while (fp != NULL && feof(fp) == 0) {
        fgets(status_line, sizeof(char) * 500, fp);
        name = strtok(status_line, ":");

        if (strcmp(name, "VmHWM") == 0)  // status 파일에 VmHWM (RES 값 있으면 받아옴)
        {
            name = strtok(NULL, "k");
            (*proc_info)->res = atoll(name);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET);  // 처음 위치로 돌림
    while (fp != NULL && feof(fp) == 0) {
        fgets(status_line, sizeof(char) * 500, fp);
        name = strtok(status_line, ":");

        if (strcmp(name, "RssFile") == 0)  // status 파일에 RssFile (SHR 값 있으면 받아옴)
        {
            name = strtok(NULL, "k");
            (*proc_info)->shr = atoll(name);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET);  // 처음 위치로 돌림
    // 이름은 무조건 있음
    fscanf(fp, "%s %s ", name, (*proc_info)->command);

    fclose(fp);
    return;
}

/* cpu 사용률 계산 + time 계산*/
void get_cpu_use(procPointer* proc_info) {
    /* uptime이 담긴 /proc/uptime 파일 열기 */
    FILE* fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/uptime file");
        exit(-1);
    }

    // cpu 사용률 계산 : ((utime+stime) / hertz) / (uptime-(startTime/hertz)) * 100
    int hertz = (int)sysconf(_SC_CLK_TCK);
    float uptime;

    fscanf(fp, "%f ", &uptime);  // uptime 계산

    // cpu 사용량
    (*proc_info)->cpu = (((*proc_info)->utime + (*proc_info)->stime) / hertz) / (uptime - ((*proc_info)->start_time / hertz)) * 100;
    // time+
    (*proc_info)->time = ((*proc_info)->utime + (*proc_info)->stime) / ((float)hertz / 100);
    float front = ((*proc_info)->time * 100 / 60) / 10000;  // 어쩌다보니 이런 공식이 나옴... (대조하면 제일 앞은 맞음)
    (*proc_info)->convert_time = (LLI)front;

    fclose(fp);
    return;
}

/* mem 사용률 계산 */
void get_mem_use(procPointer* proc_info) {
    /* memTotal이 담긴 /proc/uptime 파일 열기 */
    FILE* fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo file");
        exit(-1);
    }

    // 메모리 사용률 계산 : RES / memTotal
    char* name = (char*)malloc(sizeof(char) * 30);
    LLI memory;

    fscanf(fp, "%s %lld ", name, &memory);
    (*proc_info)->mem = ((*proc_info)->res / (float)memory) * 100;

    fclose(fp);
    return;
}
