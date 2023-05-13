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
    time_t time;
    char name[32];
} name_time;

int record_compare(const void*, const void*, void*);
bool record_iter(const void*, void*);
uint64_t record_hash(const void*, uint64_t, uint64_t);
void setup();
void cleanup();
void get_user_process();
bool is_number_string(char*);
bool is_user_process(struct stat*, char*, int);
void get_process_name_by_pid_string(char*, char*);
void compare_curr_prev();
void process_executed(char*, time_t, time_t);
void process_running(char*, time_t, time_t);
void process_terminated(char*, time_t, time_t);
struct hashmap* get_total_usage_time();
void read_map_from_file(struct hashmap*, char*);
void write_map_to_file(struct hashmap*, char*);
void read_user_process_from_file();
void write_user_process_to_file();
void scan_maps();
time_t get_start_time_today(time_t, time_t);

struct hashmap* start_time_curr;
struct hashmap* start_time_prev;
struct hashmap* usage_time_accumulated;
struct hashmap* usage_time_from_runtime;
