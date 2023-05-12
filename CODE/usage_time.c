#include "usage_time.h"

int record_compare(const void* a, const void* b, void* rdata) {
    const name_time* ra = a;
    const name_time* rb = b;
    return strcmp(ra->name, rb->name);
}

bool record_iter(const void* item, void* rdata) {
    const name_time* record = item;
    struct tm* date = localtime(&(record->time));
    printf("%s, started at %d/%d/%d %d:%d:%d\n", record->name, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
    return true;
}

uint64_t record_hash(const void* item, uint64_t seed0, uint64_t seed1) {
    const name_time* record = item;
    return hashmap_sip(record->name, strlen(record->name), seed0, seed1);
}

void setup() {
    start_time_curr = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    start_time_prev = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    usage_time_accumulated = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    usage_time_from_runtime = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
}

void cleanup() {
    hashmap_free(start_time_curr);
    hashmap_free(start_time_prev);
    hashmap_free(usage_time_accumulated);
    hashmap_free(usage_time_from_runtime);
}

void get_user_process() {
    uid_t uid;
    DIR* dir_ptr;
    struct dirent* dirent_ptr;
    struct stat statbuf;
    name_time record;
    name_time record_arg;
    name_time* record_ptr;

    uid = geteuid();

    if ((dir_ptr = opendir("/proc")) == NULL) {
        perror("opendir");
        exit(1);
    }

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (is_number_string(dirent_ptr->d_name)) {
            if (is_user_process(&statbuf, dirent_ptr->d_name, uid)) {
                record.time = statbuf.st_ctime;
                get_process_name_by_pid_string(record.name, dirent_ptr->d_name);

                strcpy(record_arg.name, record.name);
                record_ptr = hashmap_get(start_time_curr, &record_arg);

                // 한 프로그램에 대해 여러 개의 프로세스가 실행된 경우, 더 일찍 실행된 프로세스를 선택함
                if (record_ptr) {
                    if (record.time < record_ptr->time) {
                        hashmap_delete(start_time_curr, &record_arg);
                        hashmap_set(start_time_curr, &record);
                    }
#ifdef DEBUG
                    puts("hashmap conflicted!!!");
#endif
                } else {
                    hashmap_set(start_time_curr, &record);
                }
            }
        }
    }

    closedir(dir_ptr);

#ifdef DEBUG
    hashmap_scan(start_time_curr, record_iter, NULL);
#endif
}

bool is_number_string(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i])) return false;
        i++;
    }
    return true;
}

bool is_user_process(struct stat* info, char* pid_str, int uid) {
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

void compare_curr_prev() {
    time_t now = time(NULL);
    size_t iter;
    void* item;
    struct hashmap* map_ptr;

    iter = 0;
    while (hashmap_iter(start_time_curr, &iter, &item)) {
        const name_time* record_ptr = item;
        if (hashmap_get(start_time_prev, record_ptr)) {
            process_running((char*)(record_ptr->name), record_ptr->time, now);
#ifdef DEBUG
            puts("both");
#endif
        } else {
            process_executed((char*)(record_ptr->name), record_ptr->time, now);
#ifdef DEBUG
            puts("start_time_curr");
#endif
        }
    }

    iter = 0;
    while (hashmap_iter(start_time_prev, &iter, &item)) {
        const name_time* record_ptr = item;
        if (!hashmap_get(start_time_curr, record_ptr)) {
            process_terminated((char*)(record_ptr->name), record_ptr->time, now);
#ifdef DEBUG
            puts("start_time_prev");
#endif
        }
    }

    map_ptr = start_time_prev;
    start_time_prev = start_time_curr;
    start_time_curr = map_ptr;
    hashmap_clear(start_time_curr, false);

#ifdef DEBUG
    printf("\n-- iterate over all records in start_time_curr (hashmap_scan) --\n");
    hashmap_scan(start_time_curr, record_iter, NULL);
    printf("\n-- iterate over all records in start_time_prev (hashmap_scan) --\n");
    hashmap_scan(start_time_prev, record_iter, NULL);
#endif
}

void process_executed(char* name, time_t start_time, time_t now) {
    name_time record;
    record.time = now - start_time;
    strcpy(record.name, name);
    hashmap_set(usage_time_from_runtime, &record);
}

void process_running(char* name, time_t start_time, time_t now) {
    name_time record;
    record.time = now - start_time;
    strcpy(record.name, name);
    hashmap_set(usage_time_from_runtime, &record);
}

void process_terminated(char* name, time_t start_time, time_t now) {
    name_time record;
    record.time = now - start_time;
    strcpy(record.name, name);
    hashmap_set(usage_time_accumulated, &record);
}

struct hashmap* get_total_usage_time() {
    struct hashmap* total;
    size_t iter;
    void* item;
    name_time record;

    total = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);

    iter = 0;
    while (hashmap_iter(usage_time_accumulated, &iter, &item)) {
        const name_time* record_accumulated = item;
        const name_time* record_from_runtime = hashmap_get(usage_time_from_runtime, record_accumulated);

        if (record_from_runtime) {
            record.time = record_accumulated->time + record_from_runtime->time;
            strcpy(record.name, record_accumulated->name);
            hashmap_set(total, &record);
        } else {
            hashmap_set(total, record_accumulated);
        }
    }

    iter = 0;
    while (hashmap_iter(usage_time_from_runtime, &iter, &item)) {
        const name_time* record_from_runtime = item;
        const name_time* record_accumulated = hashmap_get(usage_time_accumulated, record_from_runtime);

        if (!record_accumulated) {
            hashmap_set(total, record_from_runtime);
        }
    }

    return total;
}

void read_map_from_file(struct hashmap* map, char* filename) {
    FILE* fp = fopen(filename, "r");
    name_time record = {
        0,
    };

    // 최초 실행 시, 로그 파일이 존재하지 않을 수 있음
    if (fp == NULL) return;

    while (!feof(fp)) {
        fscanf(fp, "%s %ld\n", record.name, &record.time);
        hashmap_set(map, &record);
    }

    fclose(fp);
}

void write_map_to_file(struct hashmap* map, char* filename) {
    FILE* fp = fopen(filename, "w");
    size_t iter = 0;
    void* item;

    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    while (hashmap_iter(map, &iter, &item)) {
        const name_time* record_ptr = item;
        fprintf(fp, "%s %ld\n", record_ptr->name, record_ptr->time);
    }

    fclose(fp);
}

void read_user_process_from_file() {
    time_t now = time(NULL);
    time_t last_accessed;
    struct tm* date = localtime(&now);
    char filename[32];
    struct hashmap* start_time_from_file;
    size_t iter = 0;
    void* item;

    FILE* fp = fopen("last_accessed", "r");
    if (fp == NULL) return;
    fscanf(fp, "%ld\n", &last_accessed);
    fclose(fp);

    get_user_process();

    sprintf(filename, "usage_time_%d.log", date->tm_wday);
    read_map_from_file(usage_time_accumulated, filename);

    // 프로그램을 시작한 시점에 이미 실행 중인 프로세스의 실행 시간 중, 누적 시간에 포함된 부분을 제외함
    start_time_from_file = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    sprintf(filename, "start_time_%d.log", date->tm_wday);
    read_map_from_file(start_time_from_file, filename);

    while (hashmap_iter(start_time_from_file, &iter, &item)) {
        const name_time* record_from_file = item;
        const name_time* record_from_memory = hashmap_get(start_time_curr, record_from_file);

        if (record_from_memory) {
            if (record_from_memory->time == record_from_file->time) {
                const name_time* record_accumulated = hashmap_get(usage_time_accumulated, record_from_file);
                name_time recordbuf;
                recordbuf.time = record_accumulated->time - (last_accessed - record_from_file->time);
                strcpy(recordbuf.name, record_from_file->name);
                hashmap_set(usage_time_accumulated, &recordbuf);
            }
        }
    }

    compare_curr_prev();
    hashmap_free(start_time_from_file);

#ifdef DEBUG
    printf("\n-- iterate over all records in start_time_curr (hashmap_scan) --\n");
    hashmap_scan(start_time_curr, record_iter, NULL);
    printf("\n-- iterate over all records in usage_time_accumulated (hashmap_scan) --\n");
    hashmap_scan(usage_time_accumulated, record_iter, NULL);
#endif
}

void write_user_process_to_file() {
    time_t now = time(NULL);
    struct tm* date = localtime(&now);
    char filename[32];
    struct hashmap* total = get_total_usage_time();

    FILE* fp = fopen("last_accessed", "w");
    fprintf(fp, "%ld\n", now);
    fclose(fp);

    sprintf(filename, "usage_time_%d.log", date->tm_wday);
    write_map_to_file(total, filename);
    hashmap_free(total);

    sprintf(filename, "start_time_%d.log", date->tm_wday);
    write_map_to_file(start_time_prev, filename);
}

void scan_maps() {
    puts("### scan start_time_curr ###");
    hashmap_scan(start_time_curr, record_iter, NULL);
    puts("");

    puts("### scan start_time_prev ###");
    hashmap_scan(start_time_prev, record_iter, NULL);
    puts("");

    puts("### scan usage_time_accumulated ###");
    hashmap_scan(usage_time_accumulated, record_iter, NULL);
    puts("");

    puts("### scan usage_time_from_runtime ###");
    hashmap_scan(usage_time_from_runtime, record_iter, NULL);
    puts("");
}

#ifdef DEBUG
int main() {
    struct stat info;
    char pid_str[32];
    char namebuf[32];
    uid_t uid;

    struct hashmap* map;
    name_time* record;
    struct tm* date;
    size_t iter;
    void* item;

    // test is_number_string
    puts(is_number_string("1234") ? "true" : "false");
    puts(is_number_string("onetwothreefour") ? "true" : "false");
    puts(is_number_string("1two3four") ? "true" : "false");
    puts(is_number_string("one2three4") ? "true" : "false");

    // test is_user_process
    uid = getegid();
    sprintf(pid_str, "%d", getpid());
    puts(is_user_process(&info, pid_str, uid) ? "true" : "false");
    sprintf(pid_str, "%d", 1);
    puts(is_user_process(&info, pid_str, uid) ? "true" : "false");

    // test get_process_name_by_pid_string
    get_process_name_by_pid_string(namebuf, "1");

    // test hashmap
    map = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);

    hashmap_set(map, &(name_time){.time = 10000, .name = "one"});
    hashmap_set(map, &(name_time){.time = 20000, .name = "two"});
    hashmap_set(map, &(name_time){.time = 30000, .name = "three"});

    printf("\n-- get some records --\n");

    record = hashmap_get(map, &(name_time){.name = "one"});
    date = localtime(&(record->time));
    printf("%s, started at %d/%d/%d %d:%d:%d\n", record->name, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);

    record = hashmap_get(map, &(name_time){.name = "two"});
    date = localtime(&(record->time));
    printf("%s, started at %d/%d/%d %d:%d:%d\n", record->name, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);

    record = hashmap_get(map, &(name_time){.name = "three"});
    date = localtime(&(record->time));
    printf("%s, started at %d/%d/%d %d:%d:%d\n", record->name, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);

    record = hashmap_get(map, &(name_time){.name = "four"});
    printf("%s\n", record ? "exists" : "not exists");

    printf("\n-- iterate over all records (hashmap_scan) --\n");
    hashmap_scan(map, record_iter, NULL);

    printf("\n-- iterate over all records (hashmap_iter) --\n");
    iter = 0;
    while (hashmap_iter(map, &iter, &item)) {
        const name_time* record = item;
        date = localtime(&(record->time));
        printf("%s, started at %d/%d/%d %d:%d:%d\n", record->name, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
    }

    hashmap_free(map);

    // setup global variables
    setup();

    // test get_user_process
    get_user_process();

    // test compare_curr_prev
    compare_curr_prev();

    // test get_total_usage_time
    get_total_usage_time();

    // test read_user_process_from_file
    read_user_process_from_file();

    // test write_user_process_to_file
    write_user_process_to_file();

    // cleanup global variables
    cleanup();
}
#endif

#ifdef WORKFLOW
int main() {
    /*==============================    workflow    ==============================*/
    setup();
    read_user_process_from_file();
    puts("read");
    scan_maps();
    puts("");
    // for (int i = 0; i < 10; i++) {
    //     sleep(10);
    get_user_process();
    puts("get");
    scan_maps();
    puts("");
    compare_curr_prev();
    puts("compare");
    scan_maps();
    puts("");
    write_user_process_to_file();
    puts("write");
    scan_maps();
    puts("");
    // }
    cleanup();
    /*============================================================================*/
}
#endif
