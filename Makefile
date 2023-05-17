.PHONY: all screen_time screen_time_daemon clean

all:
	make screen_time
	make screen_time_daemon

screen_time: demo_usage_time_addlimit.o usage_time.o hashmap.o
	gcc -o screen_time demo_usage_time_addlimit.o usage_time.o hashmap.o -lcurses

screen_time_daemon: screen_time_daemon.o usage_time.o hashmap.o
	gcc -o screen_time_daemon screen_time_daemon.o usage_time.o hashmap.o

demo_usage_time_addlimit.o: demo_usage_time_addlimit.c CODE/usage_time.h
	gcc -Wall -g -c demo_usage_time_addlimit.c

screen_time_daemon.o: screen_time_daemon.c CODE/usage_time.h
	gcc -Wall -g -c screen_time_daemon.c

usage_time.o: CODE/usage_time.c CODE/usage_time.h hashmap.c/hashmap.h
	gcc -Wall -g -c CODE/usage_time.c

hashmap.o: hashmap.c/hashmap.c hashmap.c/hashmap.h
	gcc -Wall -g -c hashmap.c/hashmap.c

clean:
	$(RM) *.o *.log screen_time screen_time_daemon
