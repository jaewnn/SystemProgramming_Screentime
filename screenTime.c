#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utmp.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <pwd.h>
#include <sys/time.h>
#include <utime.h>
#include <dirent.h>
#include <unistd.h>


/* 연결리스트 형태 */
typedef unsigned long long int LLI;
typedef struct proc* procPointer;
typedef struct proc {

    unsigned long pid;      // 현재 실행중인 process id
    unsigned long uid;      // 유저 id
    char user_name[30];    // 유저 이름
    LLI priority;           // PI (우선순위)
    LLI nice;               // NI (NICE 값)
    LLI virt;               // VIRT (KB기준)
    LLI res;                // RES
    LLI shr;                // SHR
    char state;             // 프로세스 상태
    LLI utime;
    LLI stime;
    LLI start_time;         // cpu사용률 계산용
    float cpu;              // cpu 사용률
    float mem;              // 메모리 사용률
    float time;             // CPU 사용시간 (0.01초 단위)
    LLI convert_time;
    char command[30];           // 프로세스 사용시 명령어


    procPointer next;       // 연결 포인터
} proc;


/* Function Declare */
procPointer make_proc();

procPointer add_process(char*);
void get_id(procPointer*);
void get_stat_file(procPointer*, char*);
void get_status_file(procPointer*, char*);
void get_cpu_use(procPointer*);
void get_mem_use(procPointer*);

void print_legend(); // 화면 출력
void print_data(procPointer);
void print_menu();

char str[1024];
char blank[1024];


int main(int argc, char* argv[]) {

    procPointer head = make_proc();
    get_id(&head);
    
    head = head->next; // proc 헤더

    initscr();
    crmode();
    noecho();
    print_legend();
    print_data(head);
    print_menu();
    refresh();

    int key;
    while (1) {
        key = getch();
        if (key == KEY_RESIZE) {
            print_legend();
            print_data(head);
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
    sprintf(str, " %6s %20s %3s %3s %7s %6s %6s %3s %3.1s %3.1s %10s %20s", "PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "%CPU", "%MEM", "TIME+", "COMMAND");
    strncpy(blank, str, strlen(str));
    move(6, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void print_data(procPointer head) {
    int i;
    memset(blank, ' ', COLS);
    for (i = 7; i < LINES - 5; i++) {

        if(strcmp("root", head->user_name) == 0)
        {
            head = head->next;
            i -= 1;
            continue;
        }

        sprintf(str, " %6ld %20s %3lld %3lld %7lld %6lld %6lld %3c %3.1f %3.1f %10lld %20s", head->pid, head->user_name, head->priority, head->nice, head->virt, head->res,
                head->shr, head->state, head->cpu, head->mem, head->convert_time, head->command);
        strncpy(blank, str, strlen(str));
        move(i, 0);
        addnstr(blank, COLS);
        head = head->next;
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

/* 출력 행 struct node 초기화 */
procPointer make_proc()
{   
    procPointer proc_node;
    proc_node =  (procPointer)malloc(sizeof(proc));
    proc_node->next = NULL;

    return proc_node;
}

/* 현재 실행중인 프로세스 정보 노드를 생성하는 함수 */
procPointer add_process(char* my_pid)
{
    struct stat         statbuf;
    struct passwd*      passbuf; // 둘 모두 UserID를 가져오기 위함
    char* proc_path = (char*)malloc(sizeof(char)*30); // progress 정보 얻는 경로
    

    procPointer new_process = make_proc();
    new_process->next = NULL;
    unsigned long pid = atoi(my_pid);
    new_process->pid = pid;             // process id

    sprintf(proc_path, "/proc/%s/stat", my_pid);
    stat(proc_path, &statbuf); // get stat
    new_process->uid = statbuf.st_uid;
    passbuf = getpwuid(new_process->uid);
    strcpy(new_process->user_name, passbuf->pw_name); // get User name

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

/* 현재 실행중인 프로세스 id 가져오기 */
void get_id(procPointer* head) 
{
    DIR*	           dir_ptr;
    struct dirent*     dirent_ptr;
    procPointer        proclist;
    

    // 현재 커널에서 작동되는 프로세스 정보를 담은 /proc 폴더 열기
    if((dir_ptr = opendir("/proc")) == NULL)
    {
        perror("/proc");
        exit(-1);
    }
    else 
    {
        proclist = *head; // 헤더 연결
        while((dirent_ptr = readdir(dir_ptr)) != NULL)
            // 파일 중에 숫자로 된 processID 이름만 받아옴. 문자 이름은 받지 않음
            if(atoi(dirent_ptr->d_name) != 0)
            {
                proclist->next = add_process(dirent_ptr->d_name);
                proclist = proclist->next;
            }
    }

    return;
}

/* proc/PID/stat file에서 정보를 읽어옴 */
void get_stat_file(procPointer* proc_info, char* path)
{
    LLI spid, ppid, sid, pgid, tty_nr, tty_pgrp, flags; // 나의 PID 부모 PID, 그룹 ID, process session ID, control하는 terminal, (모름), 리눅스 플래그
    LLI min_flt, cmin_flt, maj_flt, cmaj_flt, utime, stimev;
    LLI cutime, cstime, priority, nice, num_threads, it_real_value;
    LLI vsize, rss; // 메모리 사이즈, (모름)
    unsigned long long start_time; // 프로세스 시작 시간
    char state; // R, S, D, Z, T (S열 : 상태)
    char* comm = (char*)malloc(sizeof(char)*256); // 실행 가능한 파일 이름


     /* process 정보가 담긴 stat 파일 열기 */
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Failed to open /proc/[pid]/stat file");
        exit(-1);
    }

    // process의 자세한 정보들을 읽어온다.
    fscanf(fp, "%lld %s %c %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %llu %lld %lld",
        &spid, comm, &state, &ppid, &pgid, &sid, &tty_nr, &tty_pgrp, &flags, &min_flt, &cmin_flt, &maj_flt, &cmaj_flt, &utime, &stimev,
        &cutime, &cstime, &priority, &nice, &num_threads, &it_real_value, &start_time, &vsize, &rss );



    (*proc_info)->priority   = priority;           // PR
    (*proc_info)->nice       = nice;               // NI
    (*proc_info)->state      = state;              // State
    (*proc_info)->utime      = utime;              // utime
    (*proc_info)->stime      = stimev;             // stime
    (*proc_info)->start_time = start_time;         // start time


    free(comm);
    fclose(fp);
}

/* proc/PID/status file에서 정보를 읽어옴 */
void get_status_file(procPointer* proc_info, char* path)
{
    /* process 정보가 담긴 status 파일 열기 */
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Failed to open /proc/[pid]/status file");
        exit(-1);
    }

    char* address, address2;
    char* status_line = (char*)malloc(sizeof(char)*500);
    char* name = (char*)malloc(sizeof(char)*50); // key를 읽어냄
    

    (*proc_info)->virt = 0; // VIRT : 가상메모리 사용량
    (*proc_info)->res = 0;  // RES  : 물리메모리 사용량
    (*proc_info)->shr = 0;  // SHR  : 공유메모리 사용량
    /* status 파일 한 줄씩 읽기 */
    while(fp != NULL && feof(fp) == 0)
    {
        fgets(status_line, sizeof(char)*500, fp);
        name = strtok(status_line, ":");
        
        if(strcmp(name, "VmSize") == 0) // status 파일에 VmSize (VIRT 값 있으면 받아옴)
        {
            name = strtok(NULL, "k");
            (*proc_info)->virt = atoll(name);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET); // 처음 위치로 돌림
    while(fp != NULL && feof(fp) == 0)
    {
        fgets(status_line, sizeof(char)*500, fp);
        name = strtok(status_line, ":");
        
        if(strcmp(name, "VmHWM") == 0) // status 파일에 VmHWM (RES 값 있으면 받아옴)
        {
            name = strtok(NULL, "k");
            (*proc_info)->res = atoll(name);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET); // 처음 위치로 돌림
    while(fp != NULL && feof(fp) == 0)
    {
        fgets(status_line, sizeof(char)*500, fp);
        name = strtok(status_line, ":");
        
        if(strcmp(name, "RssFile") == 0) // status 파일에 RssFile (SHR 값 있으면 받아옴)
        {
            name = strtok(NULL, "k");
            (*proc_info)->shr = atoll(name);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET); // 처음 위치로 돌림
    // 이름은 무조건 있음
    fscanf(fp, "%s %s ", name, (*proc_info)->command);

    fclose(fp);
    return;

}

/* cpu 사용률 계산 + time 계산*/
void get_cpu_use(procPointer* proc_info)
{
    /* uptime이 담긴 /proc/uptime 파일 열기 */
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/uptime file");
        exit(-1);
    }

    // cpu 사용률 계산 : ((utime+stime) / hertz) / (uptime-(startTime/hertz)) * 100
    int hertz = (int)sysconf(_SC_CLK_TCK);
    float uptime;
    
    fscanf(fp, "%f ", &uptime); // uptime 계산

    // cpu 사용량
    (*proc_info)->cpu = (((*proc_info)->utime + (*proc_info)->stime) / hertz) / (uptime-((*proc_info)->start_time / hertz)) * 100;
    // time+
    (*proc_info)->time = ((*proc_info)->utime + (*proc_info)->stime) / ((float)hertz / 100);
    float front = ((*proc_info)->time * 100 / 60)/10000; // 어쩌다보니 이런 공식이 나옴... (대조하면 제일 앞은 맞음)
    (*proc_info)->convert_time = (LLI)front;

    fclose(fp);
    return;
}

/* mem 사용률 계산 */
void get_mem_use(procPointer* proc_info)
{
    /* memTotal이 담긴 /proc/uptime 파일 열기 */
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo file");
        exit(-1);
    }

    // 메모리 사용률 계산 : RES / memTotal
    char* name = (char*)malloc(sizeof(char)*30);
    LLI memory;

    fscanf(fp, "%s %lld ", name, &memory);
    (*proc_info)->mem = ((*proc_info)->res / (float)memory)*100;

    fclose(fp);
    return;
}

