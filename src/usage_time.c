#include "usage_time.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../hashmap.c/hashmap.h"

#define ROOT_UID 0

#define PATH_BUF_SIZE 512
#define NAME_BUF_SIZE 32
#define MQ_BUF_SIZE 64

int mday;
struct hashmap *usage_time;
struct hashmap *curr;
struct hashmap *prev;
struct hashmap *lefted;
struct hashmap *exe;
struct hashmap *exclude;
mqd_t mq;

int record_compare(const void *a, const void *b, void *rdata) {
    const name_time *ra = a;
    const name_time *rb = b;
    return strcmp(ra->name, rb->name);
}

bool start_time_iter(const void *item, void *rdata) {
    const name_time *start_time_ptr = item;
    struct tm *start_time_tm_ptr = localtime(&(start_time_ptr->time));
    printf("%s, started at %d/%d/%d %d:%d:%d\n", start_time_ptr->name, start_time_tm_ptr->tm_year + 1900,
           start_time_tm_ptr->tm_mon + 1, start_time_tm_ptr->tm_mday, start_time_tm_ptr->tm_hour,
           start_time_tm_ptr->tm_min, start_time_tm_ptr->tm_sec);
    return true;
}

bool usage_time_iter(const void *item, void *rdata) {
    const name_time *usage_time_ptr = item;
    struct tm *usage_time_tm_ptr = localtime(&(usage_time_ptr->time));
    printf("%s, used for %d:%d:%d\n", usage_time_ptr->name,
           (usage_time_tm_ptr->tm_mday - 1) * 24 + usage_time_tm_ptr->tm_hour - 9, usage_time_tm_ptr->tm_min,
           usage_time_tm_ptr->tm_sec);
    return true;
}

uint64_t record_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const name_time *record_ptr = item;
    return hashmap_sip(record_ptr->name, strlen(record_ptr->name), seed0, seed1);
}

void setup_map() {
    usage_time = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    curr = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    prev = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    lefted = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    exe = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    exclude = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);

    read_map_from_file(lefted, "log/left_time.log");
    read_map_from_file(exe, "log/exe.log");
    read_map_from_file(exclude, "log/exclude_process.log");

    mq = -1;
}

void cleanup_map() {
    restore_exe_permission();
    hashmap_free(usage_time);
    hashmap_free(curr);
    hashmap_free(prev);
    hashmap_free(lefted);
    hashmap_free(exe);
    hashmap_free(exclude);
    puts("\ngoodbye...");
    exit(EXIT_SUCCESS);
}

bool is_number_string(char *str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i]))
            return false;
        i++;
    }
    return true;
}

bool is_user_process(struct stat *stat_ptr, char *pid_str) {
    char pid_path[PATH_BUF_SIZE];
    sprintf(pid_path, "/proc/%s", pid_str);

    stat(pid_path, stat_ptr);

    return stat_ptr->st_uid != ROOT_UID ? true : false;
}

pid_t get_pid_by_process_name(char *procname) {
    pid_t result = -1;

    DIR *dir_ptr = opendir("/proc");
    if (dir_ptr == NULL) {
        perror("opendir");
        exit(1);
    }

    struct dirent *dirent_ptr;
    char name_buf[NAME_BUF_SIZE];

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (is_number_string(dirent_ptr->d_name)) {
            char status_path[PATH_BUF_SIZE];
            sprintf(status_path, "/proc/%s/status", dirent_ptr->d_name);

            FILE *fp = fopen(status_path, "r");
            fscanf(fp, "%s", name_buf);
            fscanf(fp, "%s", name_buf);
            fclose(fp);
        }

        if (strcmp(name_buf, procname) == 0) {
            result = atoi(dirent_ptr->d_name);
            break;
        }
    }

    closedir(dir_ptr);
    return result;
}

void get_process_name_by_pid_string(char *buf_ptr, char *pid_str) {
    char status_path[PATH_BUF_SIZE];
    sprintf(status_path, "/proc/%s/status", pid_str);

    FILE *fp = fopen(status_path, "r");
    fscanf(fp, "%s", buf_ptr);
    fscanf(fp, "%s", buf_ptr);

    fclose(fp);
}

void get_user_process() {
    DIR *dir_ptr = opendir("/proc");
    if (dir_ptr == NULL) {
        perror("opendir");
        exit(1);
    }

    struct dirent *dirent_ptr;
    struct stat stat_buf;
    name_time curr_buf;
    name_time *curr_ptr;

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (is_number_string(dirent_ptr->d_name)) {
            if (is_user_process(&stat_buf, dirent_ptr->d_name)) {
                curr_buf.time = stat_buf.st_ctime;
                get_process_name_by_pid_string(curr_buf.name, dirent_ptr->d_name);

                curr_ptr = hashmap_get(curr, &curr_buf);

                // 한 프로그램에 대해 여러 개의 프로세스가 실행된 경우, 더 일찍 실행된 프로세스를 선택함
                if (curr_ptr) {
                    if (curr_buf.time < curr_ptr->time) {
                        hashmap_delete(curr, curr_ptr);
                        hashmap_set(curr, &curr_buf);
                    }
                } else {
                    hashmap_set(curr, &curr_buf);
                }
            }
        }
    }

    closedir(dir_ptr);
}

void write_map_to_file(struct hashmap *map, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    size_t iter = 0;
    void *item;
    while (hashmap_iter(map, &iter, &item)) {
        const name_time *record_ptr = item;
        fprintf(fp, "%s %ld\n", record_ptr->name, record_ptr->time);
    }

    fclose(fp);
}

void read_map_from_file(struct hashmap *map, char *filename) {
    // 최초 실행 시, 로그 파일이 존재하지 않을 수 있음
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        return;

    name_time record_buf = {.time = 0, .name = ""};
    while (!feof(fp)) {
        fscanf(fp, "%s %ld\n", record_buf.name, &record_buf.time);
        hashmap_set(map, &record_buf);
    }

    fclose(fp);
}

void write_access_time_to_file(time_t access_time) {
    FILE *fp = fopen("log/access_time.log", "w");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(fp, "%ld\n", access_time);
    fclose(fp);
}

time_t read_access_time_from_file() {
    time_t access_time;

    FILE *fp = fopen("log/access_time.log", "r");
    if (fp == NULL)
        return -1; // 최초 실행 시

    fscanf(fp, "%ld\n", &access_time);
    fclose(fp);

    return access_time;
}

time_t get_start_time_today(time_t start_time, time_t now) {
    struct tm *start_time_tm_ptr = localtime(&start_time);
    int start_time_mday = start_time_tm_ptr->tm_mday;

    struct tm *now_tm_ptr = localtime(&now);
    int now_mday = now_tm_ptr->tm_mday;

    if (start_time_mday == now_mday)
        return start_time;

    now_tm_ptr->tm_sec = 0;
    now_tm_ptr->tm_min = 0;
    now_tm_ptr->tm_hour = 0;

    time_t start_time_today = mktime(now_tm_ptr);

    return start_time_today;
}

void swap_curr_prev() {
    struct hashmap *tmp;
    tmp = curr;
    curr = prev;
    prev = tmp;
}

void read_usage_time_from_file() {
    time_t now = time(NULL);
    struct tm *now_tm_ptr = localtime(&now);
    mday = now_tm_ptr->tm_mday;

    char filename[NAME_BUF_SIZE];
    sprintf(filename, "log/usage_time_%d.log", now_tm_ptr->tm_wday);
    read_map_from_file(usage_time, filename);

    time_t access_time = read_access_time_from_file();

    get_user_process();

    size_t iter = 0;
    void *item;
    while (hashmap_iter(curr, &iter, &item)) {
        const name_time *start_time_ptr = item;
        const name_time *usage_time_ptr = hashmap_get(usage_time, start_time_ptr);
        name_time usage_time_buf;

        strcpy(usage_time_buf.name, start_time_ptr->name);
        usage_time_buf.time = usage_time_ptr ? usage_time_ptr->time : 0;

        if (start_time_ptr->time < access_time)
            usage_time_buf.time += now - get_start_time_today(access_time, now);
        else
            usage_time_buf.time += now - get_start_time_today(start_time_ptr->time, now);

        hashmap_set(usage_time, &usage_time_buf);
    }

    write_map_to_file(usage_time, filename);
    write_access_time_to_file(now);

    swap_curr_prev();
    hashmap_clear(curr, false);
}

void write_usage_time_to_file() {
    time_t now = time(NULL);
    struct tm *now_tm_ptr = localtime(&now);

    char filename[NAME_BUF_SIZE];
    sprintf(filename, "log/usage_time_%d.log", now_tm_ptr->tm_wday);

    time_t access_time = read_access_time_from_file();
    if (access_time == -1)
        access_time = now;

    get_user_process();

    size_t iter = 0;
    void *item;
    while (hashmap_iter(curr, &iter, &item)) {
        const name_time *start_time_ptr = item;
        const name_time *usage_time_ptr = hashmap_get(usage_time, start_time_ptr);
        name_time usage_time_buf;

        strcpy(usage_time_buf.name, start_time_ptr->name);
        usage_time_buf.time = usage_time_ptr ? usage_time_ptr->time : 0;

        if (hashmap_get(prev, start_time_ptr)) {
            usage_time_buf.time += now - get_start_time_today(access_time, now);
        } else
            usage_time_buf.time += now - get_start_time_today(start_time_ptr->time, now);

        hashmap_set(usage_time, &usage_time_buf);
    }

    iter = 0;
    while (hashmap_iter(exclude, &iter, &item)) {
        const name_time *exclude_ptr = item;
        if (hashmap_get(usage_time, exclude_ptr))
            hashmap_delete(usage_time, (void *)exclude_ptr);
    }

    write_map_to_file(usage_time, filename);
    write_access_time_to_file(now);

    swap_curr_prev();
    hashmap_clear(curr, false);
}

void update_lefted(time_t interval) {
    size_t iter = 0;
    void *item;
    while (hashmap_iter(lefted, &iter, &item)) {
        const name_time *lefted_ptr = item;
        const name_time *prev_ptr = hashmap_get(prev, lefted_ptr);
        name_time lefted_buf;

        if (prev_ptr) {
            strcpy(lefted_buf.name, lefted_ptr->name);
            lefted_buf.time = lefted_ptr->time - interval;

            hashmap_set(lefted, &lefted_buf);
            if (lefted_buf.time <= 0) {
                pid_t pid = get_pid_by_process_name(lefted_buf.name);
                if (pid == -1)
                    continue;

                char link_path_buf[PATH_BUF_SIZE];
                sprintf(link_path_buf, "/proc/%d/exe", pid);

                char exe_path_buf[PATH_BUF_SIZE];
                memset(exe_path_buf, 0, PATH_BUF_SIZE);
                readlink(link_path_buf, exe_path_buf, PATH_BUF_SIZE);

                struct stat stat_buf;
                stat(exe_path_buf, &stat_buf);

                name_time exe_buf;
                memset(&exe_buf, 0, sizeof(name_time));
                strcpy(exe_buf.name, exe_path_buf);
                hashmap_set(exe, &exe_buf);
                write_map_to_file(exe, "log/exe.log");

                mode_t mode = stat_buf.st_mode;
                mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);
                chmod(exe_path_buf, mode);

                kill(pid, SIGTERM);
            }
        }
    }

    write_map_to_file(lefted, "log/left_time.log");
}

void add_exclude_process_by_name(char *procname) {
    name_time exclude_buf;
    memset(&exclude_buf, 0, sizeof(name_time));
    strcpy(exclude_buf.name, procname);
    hashmap_set(exclude, &exclude_buf);
    write_map_to_file(exclude, "log/exclude_process.log");
}

void add_time_lefted(char *procname, int time_lefted) {
    name_time lefted_buf;
    memset(&lefted_buf, 0, sizeof(name_time));
    strcpy(lefted_buf.name, procname);

    const name_time *usage_time_ptr = hashmap_get(usage_time, &lefted_buf);
    lefted_buf.time = time_lefted - usage_time_ptr->time;

    if (lefted_buf.time <= 0)
        lefted_buf.time = 0;

    hashmap_set(lefted, &lefted_buf);
}

void process_message() {
    if (mq == -1) {
        mq = mq_open("/screen_time_mq", O_RDONLY | O_NONBLOCK);
        if (mq == -1)
            return;
    }

    char mq_buf[MQ_BUF_SIZE];
    int result = mq_receive(mq, mq_buf, MQ_BUF_SIZE, NULL);
    if (result == -1)
        return;

    char *func = strtok(mq_buf, " ");

    if (strcmp(func, "time_lefted") == 0) {
        char *procname = strtok(NULL, " ");
        int time_lefted = atoi(strtok(NULL, " "));
        add_time_lefted(procname, time_lefted);
    }

    if (strcmp(func, "exclude") == 0) {
        char *procname = strtok(NULL, " ");
        add_exclude_process_by_name(procname);
    }
}

void process_day_changed() {
    time_t now = time(NULL);
    struct tm *now_tm_ptr = localtime(&now);

    if (mday != now_tm_ptr->tm_mday) {
        mday = now_tm_ptr->tm_mday;
        restore_exe_permission();
        hashmap_clear(usage_time, false);
        hashmap_clear(lefted, false);
        hashmap_clear(exe, false);
    }
}

void restore_exe_permission() {
    size_t iter = 0;
    void *item;
    while (hashmap_iter(exe, &iter, &item)) {
        const name_time *exe_ptr = item;

        struct stat stat_buf;
        stat(exe_ptr->name, &stat_buf);

        mode_t mode = stat_buf.st_mode;
        mode |= S_IXUSR | S_IXGRP | S_IXOTH;
        chmod(exe_ptr->name, mode);
    }
}
