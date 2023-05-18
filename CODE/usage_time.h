#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#include "../hashmap.c/hashmap.h"

typedef struct {
    time_t time;
    char name[32];
} name_time;

int record_compare(const void* a, const void* b, void* rdata);
bool start_time_iter(const void* item, void* rdata);
bool usage_time_iter(const void* item, void* rdata);
uint64_t record_hash(const void* item, uint64_t seed0, uint64_t seed1);

void setup_map();
void cleanup_map();

bool is_number_string(char* str);
bool is_user_process(struct stat* stat_ptr, char* pid_str, int uid);
void get_process_name_by_pid_string(char* buf_ptr, char* pid_str);
void get_user_process();

void write_map_to_file(struct hashmap* map, char* filename);
void read_map_from_file(struct hashmap* map, char* filename);
void write_access_time_to_file(time_t access_time);
time_t read_access_time_from_file();

time_t get_start_time_today(time_t start_time, time_t now);
void swap_curr_prev();
void read_usage_time_from_file();
void write_usage_time_to_file();

int mday;
struct hashmap* usage_time;
struct hashmap* curr;
struct hashmap* prev;
struct hashmap* exclude;
