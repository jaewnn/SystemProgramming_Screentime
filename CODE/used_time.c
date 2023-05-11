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

char user[32];

void get_user_name() {
    uid_t uid = getegid();
    struct passwd* pwd = getpwuid(uid);
    strcpy(user, pwd->pw_name);
}

#ifdef DEBUG
int main() {
    get_user_name();
    puts(user);
}
#endif
