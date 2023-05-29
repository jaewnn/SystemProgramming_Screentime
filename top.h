#ifndef __TOP_H__
#define __TOP_H__


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
} proc;


procPointer make_proc();
int top_get_process_name_by_pid_string(char*, char*);
void get_info_from_name(procPointer*, char*);

void add_process(char*, procPointer*);
void get_stat_file(procPointer*, char*);
void get_status_file(procPointer*, char*);

void get_cpu_use(procPointer*);
void get_mem_use(procPointer*);

#endif
