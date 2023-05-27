#include <curses.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "screen_time_func.h"
#include "usage_time.h"

char str[1024];
char blank[1024];

data data_arr[MAX_DATA_LINES];
int data_count;

char mq_buf[MQ_BUF_SIZE];
char name_buf[NAME_BUF_SIZE];
time_t time_lefted_buf;

time_t prev_usage_time[WEEK];

int compare(const void *a, const void *b) {
    const data *da = a;
    const data *db = b;
    return db->usage_time - da->usage_time;
}

void read_prev_usage_time() {
    memset(prev_usage_time, 0, sizeof(time_t) * WEEK);

    time_t now = time(NULL);
    struct tm *now_tm_ptr = localtime(&now);

    for (int i = 0; i < WEEK; i++) {
        if (i == now_tm_ptr->tm_wday)
            continue;

        char filename[32];
        sprintf(filename, "log/usage_time_%d.log", i);

        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
            continue;

        char name_buf[NAME_BUF_SIZE];
        time_t time_buf;
        while (!feof(fp)) {
            fscanf(fp, "%s %ld\n", name_buf, &time_buf);
            prev_usage_time[i] += time_buf;
        }

        fclose(fp);
    }
}

void select_process_by_number() {
    nocbreak();  // enable canonical input mode
    echo();      // enable echo
    timeout(-1); // blocking

    move(LINES - INPUT_LINE_FROM_BOTTOM, 0);
    addnstr("# of process: ", COLS);
    refresh();

    int number_of_process;
    int result = scanw("%d", &number_of_process);

    noecho(); // disable echo
    cbreak(); // disable canonical input mode

    memset(name_buf, 0, NAME_BUF_SIZE);
    if (result == -1 || number_of_process - 1 < 0 || number_of_process - 1 >= data_count ||
        number_of_process >= LINES - DATA_LINE_FROM_TOP - DATA_LINE_FROM_BOTTOM + 1) {
        move(LINES - INPUT_LINE_FROM_BOTTOM + 1, 0);
        addnstr("Enter valid number. Press any key to continue...", COLS);
        getch();
    } else {
        strcpy(name_buf, data_arr[number_of_process - 1].name);
    }

    timeout(0); // non-blocking
}

void select_time_lefted() {
    nocbreak();  // enable canonical input mode
    echo();      // enable echo
    timeout(-1); // blocking

    move(LINES - INPUT_LINE_FROM_BOTTOM, 0);
    addnstr("Please enter a time limit (sec): ", COLS);
    refresh();

    int result = scanw("%ld", &time_lefted_buf);

    noecho(); // disable echo
    cbreak(); // disable canonical input mode

    if (result == -1 || time_lefted_buf < 0) {
        time_lefted_buf = -1;
        move(LINES - INPUT_LINE_FROM_BOTTOM + 1, 0);
        addnstr("Enter valid number. Press any key to continue...", COLS);
        getch();
    }

    timeout(0); // non-blocking
}

void set_time_limits() {
    select_process_by_number();
    if (name_buf[0] == '\0')
        return;

    select_time_lefted();
    if (time_lefted_buf == -1)
        return;

    struct mq_attr attr = {.mq_maxmsg = 4, .mq_msgsize = MQ_BUF_SIZE};
    mqd_t mq = mq_open("/screen_time_mq", O_WRONLY | O_CREAT, 0666, &attr);

    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    sprintf(mq_buf, "time_lefted %s %ld", name_buf, time_lefted_buf);

    int result = mq_send(mq, mq_buf, MQ_BUF_SIZE, 0);
    if (result == -1) {
        perror("mq_send");
        mq_close(mq);
        exit(1);
    }

    mq_close(mq);
}

void exclude_from_list() {
    select_process_by_number();
    if (name_buf[0] == '\0')
        return;

    struct mq_attr attr = {.mq_maxmsg = 4, .mq_msgsize = MQ_BUF_SIZE};
    mqd_t mq = mq_open("/screen_time_mq", O_WRONLY | O_CREAT, 0666, &attr);

    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    sprintf(mq_buf, "exclude %s", name_buf);

    int result = mq_send(mq, mq_buf, MQ_BUF_SIZE, 0);
    if (result == -1) {
        perror("mq_send");
        mq_close(mq);
        exit(1);
    }

    mq_close(mq);
}

void read_usage_time() {
    memset(data_arr, 0, sizeof(name_time) * MAX_DATA_LINES);
    data_count = 0;

    time_t now = time(NULL);
    struct tm *now_tm_ptr = localtime(&now);

    char filename[32];
    sprintf(filename, "log/usage_time_%d.log", now_tm_ptr->tm_wday);

    prev_usage_time[now_tm_ptr->tm_wday] = 0;

    struct hashmap *usage_time = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);
    struct hashmap *lefted = hashmap_new(sizeof(name_time), 0, 0, 0, record_hash, record_compare, NULL, NULL);

    read_map_from_file(usage_time, filename);
    read_map_from_file(lefted, "log/left_time.log");

    size_t iter = 0;
    void *item;
    while (hashmap_iter(usage_time, &iter, &item)) {
        const name_time *usage_time_ptr = item;
        const name_time *lefted_ptr = hashmap_get(lefted, usage_time_ptr);

        if (data_count < MAX_DATA_LINES) {
            strcpy(data_arr[data_count].name, usage_time_ptr->name);
            data_arr[data_count].usage_time = usage_time_ptr->time;

            if (lefted_ptr) {
                if (lefted_ptr->time >= 0)
                    data_arr[data_count].time_lefted = lefted_ptr->time;
                else
                    data_arr[data_count].time_lefted = 0;
            } else {
                data_arr[data_count].time_lefted = -1;
            }
        }

        prev_usage_time[now_tm_ptr->tm_wday] += usage_time_ptr->time;
        data_count++;
    }

    hashmap_free(usage_time);
    hashmap_free(lefted);

    qsort(data_arr, MAX_DATA_LINES, sizeof(data), compare);
}

void print_access_time() {
    FILE *fp = fopen("log/access_time.log", "r");
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
    struct tm *now_tm_ptr = localtime(&now);

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
    sprintf(str, " %6s %-32s %-16s %-16s ", "No.", "Apps", "Time", "Limits");
    strncpy(blank, str, strlen(str));
    move(LEGEND_LINE_FROM_TOP, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}

void print_data() {
    for (int i = DATA_LINE_FROM_TOP; i < LINES - DATA_LINE_FROM_BOTTOM; i++) {
        memset(blank, ' ', COLS);
        int data_index = i - DATA_LINE_FROM_TOP;

        if (data_index < data_count) {
            struct tm *usage_time_tm_ptr = localtime(&data_arr[data_index].usage_time);
            sprintf(str, " %6d %-32s %02d:%02d:%02d         ", data_index + 1, data_arr[data_index].name,
                    (usage_time_tm_ptr->tm_mday - 1) * 24 + usage_time_tm_ptr->tm_hour - 9, usage_time_tm_ptr->tm_min,
                    usage_time_tm_ptr->tm_sec);

            if (data_arr[data_index].time_lefted == -1) {
                sprintf(str + strlen(str), "--:--:-- ");
            } else {
                struct tm *time_lefted_tm_ptr = localtime(&data_arr[data_index].time_lefted);
                sprintf(str + strlen(str), "%02d:%02d:%02d ",
                        (time_lefted_tm_ptr->tm_mday - 1) * 24 + time_lefted_tm_ptr->tm_hour - 9,
                        time_lefted_tm_ptr->tm_min, time_lefted_tm_ptr->tm_sec);
            }

            strncpy(blank, str, strlen(str));
        }

        move(i, 0);
        addnstr(blank, COLS);
    }

    memset(blank, '-', COLS);
    move(LINES - DATA_LINE_FROM_BOTTOM, 0);
    addnstr(blank, COLS);
    refresh();
}

void print_menu() {
    memset(blank, ' ', COLS);
    for (int i = LINES - INPUT_LINE_FROM_BOTTOM; i < LINES - 1; i++) {
        move(i, 0);
        addnstr(blank, COLS);
    }

    sprintf(str, "1) Set time limits    2) Exclude from list    3) Exit");
    strncpy(blank, str, strlen(str));
    move(LINES - 1, 0);
    standout();
    addnstr(blank, COLS);
    standend();
    refresh();
}
