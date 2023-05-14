/*
gcc -o usage_time usage_time.c ../hashmap.c/hashmap.c
 */

#include "usage_time.h"

int record_compare(const void* a, const void* b, void* rdata) {
    const name_time* ra = a;
    const name_time* rb = b;
    return strcmp(ra->name, rb->name);
}

bool start_time_iter(const void* item, void* rdata) {
    const name_time* start_time_ptr = item;
    struct tm* start_time_tm_ptr = localtime(&(start_time_ptr->time));
    printf("%s, started at %d/%d/%d %d:%d:%d\n",
           start_time_ptr->name,
           start_time_tm_ptr->tm_year + 1900,
           start_time_tm_ptr->tm_mon + 1,
           start_time_tm_ptr->tm_mday,
           start_time_tm_ptr->tm_hour,
           start_time_tm_ptr->tm_min,
           start_time_tm_ptr->tm_sec);
    return true;
}

bool usage_time_iter(const void* item, void* rdata) {
    const name_time* usage_time_ptr = item;
    struct tm* usage_time_tm_ptr = localtime(&(usage_time_ptr->time));
    printf("%s, used for %d:%d:%d\n",
           usage_time_ptr->name,
           (usage_time_tm_ptr->tm_mday - 1) * 24 + usage_time_tm_ptr->tm_hour - 9,
           usage_time_tm_ptr->tm_min,
           usage_time_tm_ptr->tm_sec);
    return true;
}

uint64_t record_hash(const void* item, uint64_t seed0, uint64_t seed1) {
    const name_time* record_ptr = item;
    return hashmap_sip(record_ptr->name, strlen(record_ptr->name), seed0, seed1);
}

void setup_map() {
    usage_time = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    curr = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    prev = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
}

void cleanup_map() {
    hashmap_free(usage_time);
    hashmap_free(curr);
    hashmap_free(prev);
    puts("\ngoodbye...");
    exit(EXIT_SUCCESS);
}

bool is_number_string(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i])) return false;
        i++;
    }
    return true;
}

bool is_user_process(struct stat* stat_ptr, char* pid_str, int uid) {
    char pid_path[32];
    sprintf(pid_path, "/proc/%s", pid_str);

    stat(pid_path, stat_ptr);

    return stat_ptr->st_uid == uid ? true : false;
}

void get_process_name_by_pid_string(char* buf_ptr, char* pid_str) {
    char status_path[32];
    sprintf(status_path, "/proc/%s/status", pid_str);

    FILE* fp = fopen(status_path, "r");
    fscanf(fp, "%s", buf_ptr);
    fscanf(fp, "%s", buf_ptr);

    fclose(fp);
}

void get_user_process() {
    DIR* dir_ptr = opendir("/proc");
    if (dir_ptr == NULL) {
        perror("opendir");
        exit(1);
    }

    struct dirent* dirent_ptr;
    struct stat stat_buf;
    uid_t uid = geteuid();
    name_time curr_buf;
    name_time* curr_ptr;

    while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
        if (is_number_string(dirent_ptr->d_name)) {
            if (is_user_process(&stat_buf, dirent_ptr->d_name, uid)) {
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

void write_map_to_file(struct hashmap* map, char* filename) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    size_t iter = 0;
    void* item;
    while (hashmap_iter(map, &iter, &item)) {
        const name_time* record_ptr = item;
        fprintf(fp, "%s %ld\n", record_ptr->name, record_ptr->time);
    }

    fclose(fp);
}

void read_map_from_file(struct hashmap* map, char* filename) {
    // 최초 실행 시, 로그 파일이 존재하지 않을 수 있음
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) return;

    name_time record_buf = {.time = 0, .name = ""};
    while (!feof(fp)) {
        fscanf(fp, "%s %ld\n", record_buf.name, &record_buf.time);
        hashmap_set(map, &record_buf);
    }

    fclose(fp);
}

void write_access_time_to_file(time_t access_time) {
    FILE* fp = fopen("access_time.log", "w");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(fp, "%ld\n", access_time);
    fclose(fp);
}

time_t read_access_time_from_file() {
    time_t access_time;

    FILE* fp = fopen("access_time.log", "r");
    if (fp == NULL) return -1;  // 최초 실행 시

    fscanf(fp, "%ld\n", &access_time);
    fclose(fp);

    return access_time;
}

time_t get_start_time_today(time_t start_time, time_t now) {
    struct tm* start_time_tm_ptr = localtime(&start_time);
    int start_time_mday = start_time_tm_ptr->tm_mday;

    struct tm* now_tm_ptr = localtime(&now);
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
    struct hashmap* tmp;
    tmp = curr;
    curr = prev;
    prev = tmp;
}

void read_usage_time_from_file() {
    time_t now = time(NULL);
    struct tm* now_tm_ptr = localtime(&now);
    mday = now_tm_ptr->tm_mday;

    char filename[32];
    sprintf(filename, "usage_time_%d.log", now_tm_ptr->tm_wday);
    read_map_from_file(usage_time, filename);

    time_t access_time = read_access_time_from_file();

    get_user_process();

    size_t iter = 0;
    void* item;
    while (hashmap_iter(curr, &iter, &item)) {
        const name_time* start_time_ptr = item;
        const name_time* usage_time_ptr = hashmap_get(usage_time, start_time_ptr);
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
    struct tm* now_tm_ptr = localtime(&now);
    if (mday != now_tm_ptr->tm_mday) {
        mday = now_tm_ptr->tm_mday;
        hashmap_clear(usage_time, false);
    }

    char filename[32];
    sprintf(filename, "usage_time_%d.log", now_tm_ptr->tm_wday);

    time_t access_time = read_access_time_from_file();
    if (access_time == -1) access_time = now;

    get_user_process();

    size_t iter = 0;
    void* item;
    while (hashmap_iter(curr, &iter, &item)) {
        const name_time* start_time_ptr = item;
        const name_time* usage_time_ptr = hashmap_get(usage_time, start_time_ptr);
        name_time usage_time_buf;

        strcpy(usage_time_buf.name, start_time_ptr->name);
        usage_time_buf.time = usage_time_ptr ? usage_time_ptr->time : 0;

        if (hashmap_get(prev, start_time_ptr)) {
            usage_time_buf.time += now - get_start_time_today(access_time, now);
        } else
            usage_time_buf.time += now - get_start_time_today(start_time_ptr->time, now);

        hashmap_set(usage_time, &usage_time_buf);
    }

    write_map_to_file(usage_time, filename);
    write_access_time_to_file(now);

    swap_curr_prev();
    hashmap_clear(curr, false);
}

int main(int argc, char* argv[]) {
    puts("*** this program should be run in background ***");

    bool execute = false;
    bool verbose = false;
    int interval;

    if (argc == 2 && is_number_string(argv[1])) {
        execute = true;
        interval = atoi(argv[1]);
    }
    if (argc == 3 && strcmp(argv[1], "-v") == 0 && is_number_string(argv[2])) {
        execute = true;
        verbose = true;
        interval = atoi(argv[2]);
    }

    if (!execute) {
        printf("Usage : %s [-v : verbose] <interval [sec]>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, cleanup_map);
    signal(SIGTERM, cleanup_map);

    setup_map();
    read_usage_time_from_file();

    while (1) {
        sleep(interval);
        write_usage_time_to_file();
        if (verbose) puts("usage time recorded...");
    }

    cleanup_map();
    return 0;
}
