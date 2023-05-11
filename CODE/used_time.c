#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG
#include <stdio.h>
#endif

typedef struct {
    int start_time;
    char name[32];
} name_start_time;

typedef struct {
    int week;
    int usage_time;
    char name[32];
} name_usage_time;

void get_user_name();
void get_user_process();
int is_number_string(char*);

char user[32];

void get_user_name() {
    uid_t uid = getegid();
    struct passwd* pwd = getpwuid(uid);
    strcpy(user, pwd->pw_name);
#ifdef DEBUG
    puts(user);
#endif
}

// void get_user_process() {
//     DIR* dir_ptr;
//     struct dirent* dirent_ptr;

//     if ((dir_ptr = opendir("/proc")) == NULL) {
//         perror("opendir");
//         exit(1);
//     }

//     while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
//     }
// }

int is_number_string(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i])) return 0;
        i++;
    }
    return 1;
}

#ifdef DEBUG
int main() {
    get_user_name();
    puts(is_number_string("1234") ? "true" : "false");
    puts(is_number_string("onetwothreefour") ? "true" : "false");
    puts(is_number_string("1two3four") ? "true" : "false");
    puts(is_number_string("one2three4") ? "true" : "false");
}
#endif
