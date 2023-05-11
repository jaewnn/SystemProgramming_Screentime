#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../hashmap.c/hashmap.h"

typedef struct {
    time_t start_time;
    char name[32];
} name_start_time;

typedef struct {
    int week;
    time_t usage_time;
    char name[32];
} name_usage_time;

int record_compare(const void*, const void*, void*);
bool record_iter(const void*, void*);
void get_user_name();
void get_user_process();
bool is_number_string(char*);
bool is_user_process(struct stat*, char*);
void get_process_name_by_pid_string(char*, char*);

uid_t uid;
char user[32];

// int record_compare(const void* a, const void* b, void* rdata) {
//     const name_start_time* ra = a;
//     const name_start_time* rb = b;
//     return strcmp(ra->name, rb->name);
// }

// bool record_iter(const void* item, void* rdata) {
//     const name_start_time* record = item;
//     struct tm* date = localtime(&(record->start_time));
//     printf("%s, started at %d/%d/%d %d:%d:%d\n", record->name, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
//     return true;
// }

// uint64_t user_hash(const void* item, uint64_t seed0, uint64_t seed1) {
//     const name_start_time* record = item;
//     return hashmap_sip(record->name, strlen(record->name), seed0, seed1);
// }

void get_user_id_and_name() {
    struct passwd* pwd;
    uid = geteuid();
    pwd = getpwuid(uid);
    strcpy(user, pwd->pw_name);
#ifdef DEBUG
    puts(user);
#endif
}

void get_user_process() {
    DIR* dir_ptr;
    struct dirent* dirent_ptr;
    struct stat statbuf;
    struct tm* date;
    time_t now;
    time_t timebuf;

    time(&now);

    if ((dir_ptr = opendir("/proc")) == NULL) {
        perror("opendir");
        exit(1);
    }

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (is_number_string(dirent_ptr->d_name)) {
            if (is_user_process(&statbuf, dirent_ptr->d_name)) {
                timebuf = now - statbuf.st_ctime;
                date = localtime(&timebuf);
#ifdef DEBUG
                printf("pid: %s, usage time: %d:%d:%d\n", dirent_ptr->d_name, (date->tm_mday - 1) * 24 + date->tm_hour - 9, date->tm_min, date->tm_sec);
#endif
            }
        }
    }

    closedir(dir_ptr);
}

bool is_number_string(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i])) return false;
        i++;
    }
    return true;
}

bool is_user_process(struct stat* info, char* pid_str) {
    char pid_path[32];
    sprintf(pid_path, "/proc/%s", pid_str);
    stat(pid_path, info);
    if (info->st_uid == uid)
        return true;
    else
        return false;
}

void get_process_name_by_pid_string(char* namebuf, char* pid_str) {
    char status_path[32];
    FILE* fp;

    sprintf(status_path, "/proc/%s/status", pid_str);
    fp = fopen(status_path, "r");
    fscanf(fp, "%s", namebuf);
    fscanf(fp, "%s", namebuf);
    fclose(fp);
#ifdef DEBUG
    puts(namebuf);
#endif
}

#ifdef DEBUG
int main() {
    struct stat info;
    char pid_str[32];
    char namebuf[32];

    // test get_user_id_and_name
    get_user_id_and_name();

    // test is_number_string
    puts(is_number_string("1234") ? "true" : "false");
    puts(is_number_string("onetwothreefour") ? "true" : "false");
    puts(is_number_string("1two3four") ? "true" : "false");
    puts(is_number_string("one2three4") ? "true" : "false");

    // test is_user_process
    sprintf(pid_str, "%d", getpid());
    puts(is_user_process(&info, pid_str) ? "true" : "false");
    sprintf(pid_str, "%d", 1);
    puts(is_user_process(&info, pid_str) ? "true" : "false");

    // test get_user_process
    get_user_process();

    // test get_process_name_by_pid_string
    get_process_name_by_pid_string(namebuf, "1");
}
#endif
