#ifndef __SCREEN_TIME_FUNC_H__
#define __SCREEN_TIME_FUNC_H__

#include <time.h>

#define MAX_DATA_LINES 64
#define ACCESS_TIME_LINE_FROM_TOP 0
#define GRAPH_LINE_FROM_TOP 2
#define LEGEND_LINE_FROM_TOP 10
#define DATA_LINE_FROM_TOP 11
#define DATA_LINE_FROM_BOTTOM 7
#define INPUT_LINE_FROM_BOTTOM 6

#define NAME_BUF_SIZE 32
#define MQ_BUF_SIZE 64

#define SET_TIME_LIMITS '1'
#define EXCLUDE_FROM_LIST '2'
#define EXIT '3'

#define WEEK 7

typedef struct {
    time_t usage_time;
    time_t time_lefted;
    char name[32];
} data;

int compare(const void *da, const void *db);

void read_prev_usage_time();
void select_process_by_number();
void select_time_lefted();
void set_time_limits();
void exclude_from_list();

void read_usage_time();
void print_access_time();
void print_graph();
void print_legend();
void print_data();
void print_menu();

#endif
