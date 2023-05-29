#include <curses.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "top.h"


procPointer make_proc() {
    procPointer proc_node;
    proc_node = (procPointer)malloc(sizeof(proc));
    proc_node->pid = 0;  // 현재 실행중인 process id
    proc_node->priority = 0;  // PI (우선순위)
    proc_node->nice = 0;      // NI (NICE 값)
    proc_node->virt = 0;      // VIRT (KB기준)
    proc_node->res = 0;       // RES
    proc_node->shr = 0;       // SHR
    proc_node->state = 'F';   // 프로세스 상태

    proc_node->cpu = 0;  // cpu 사용률
    proc_node->mem = 0;  // 메모리 사용률

    return proc_node;
}

int top_get_process_name_by_pid_string(char* target_name, char* pid)
{
	char path[50];
	FILE* file;
	
	sprintf(path, "/proc/%s/status", pid);
	
	if((file = fopen(path, "r")) == NULL)
	{
		perror("Fail to open status file");
		return -1;
	}
	
	// read name
	fscanf(file, "%s", target_name);
	fscanf(file, "%s", target_name);
	
	fclose(file);

	return 0;
}

void get_info_from_name(procPointer* info, char* name) {
    
    DIR* dir_ptr;
    struct dirent* dirent_ptr;
    char pid_name[256];
    char pid[30];


    if ((dir_ptr = opendir("/proc")) == NULL) {
        perror("opendir");
        exit(1);
    }

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (atoi(dirent_ptr->d_name) != 0) {
            top_get_process_name_by_pid_string(pid_name, dirent_ptr->d_name);

            if (strcmp(pid_name, name) == 0) {
                strcpy(pid, dirent_ptr->d_name);
                break;
            }
            memset(pid_name, 0, 30);
        }
    }

    if (atoll(pid) != 0)
        add_process(pid, info);

    closedir(dir_ptr);
}

/* 현재 실행중인 프로세스 정보 노드를 생성하는 함수 */
void add_process(char* my_pid, procPointer* info) {
    struct stat statbuf;
    struct passwd* passbuf;                              // 둘 모두 UserID를 가져오기 위함
    char proc_path[30];  // progress 정보 얻는 경로

    unsigned long pid = atoll(my_pid);
    (*info)->pid = pid;  // process id

    sprintf(proc_path, "/proc/%s/stat", my_pid);
    stat(proc_path, &statbuf);  // get stat
    (*info)->uid = statbuf.st_uid;
    passbuf = getpwuid((*info)->uid);
    strcpy((*info)->user_name, passbuf->pw_name);  // get User name

    /* Get all information from stat file : PR, NI, state */
    get_stat_file(info, proc_path);

    /* Get all information from status file : VIRT, RES, SHR, Command */
    sprintf(proc_path, "/proc/%s/status", my_pid);
    get_status_file(info, proc_path);

    /* Bring Cpu and Memory using rate */
    get_cpu_use(info);
    get_mem_use(info);

    return;
}

/* proc/PID/stat file에서 정보를 읽어옴 */
void get_stat_file(procPointer* proc_info, char* path) {
    LLI spid, ppid, sid, pgid, tty_nr, tty_pgrp, flags;  // 나의 PID 부모 PID, 그룹 ID, process session ID, control하는 terminal, (모름), 리눅스 플래그
    LLI min_flt, cmin_flt, maj_flt, cmaj_flt, utime, stimev;
    LLI cutime, cstime, priority, nice, num_threads, it_real_value;
    LLI vsize, rss;                                  // 메모리 사이즈, (모름)
    unsigned long long start_time;                   // 프로세스 시작 시간
    char state;                                      // R, S, D, Z, T (S열 : 상태)
    char comm[256];  // 실행 가능한 파일 이름

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

    char status_line[500];
    char* name;  // key를 읽어냄

    (*proc_info)->virt = 0;  // VIRT : 가상메모리 사용량
    (*proc_info)->res = 0;   // RES  : 물리메모리 사용량
    (*proc_info)->shr = 0;   // SHR  : 공유메모리 사용량
    /* status 파일 한 줄씩 읽기 */
    while (fp != NULL && feof(fp) == 0) {
        fgets(status_line, 500, fp);
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
        fgets(status_line, 500, fp);
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
        fgets(status_line, 500, fp);
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
    char name[30];
    LLI memory;

    fscanf(fp, "%s %lld ", name, &memory);
    (*proc_info)->mem = ((*proc_info)->res / (float)memory) * 100;

    fclose(fp);
    return;
}

