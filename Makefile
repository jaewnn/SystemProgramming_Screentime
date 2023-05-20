.PHONY: all screen_time screen_time_daemon clean

all:
	make screen_time
	make screen_time_daemon
	make left_time.log

screen_time: screen_time.o usage_time.o hashmap.o timelimit.o
	sudo gcc -o screen_time screen_time.o usage_time.o hashmap.o timelimit.o -lcurses -lrt

screen_time_daemon: screen_time_daemon.o usage_time.o hashmap.o
	gcc -o screen_time_daemon screen_time_daemon.o usage_time.o hashmap.o -lrt

screen_time.o: screen_time.c CODE/usage_time.h
	sudo gcc -Wall -g -c screen_time.c

screen_time_daemon.o: screen_time_daemon.c CODE/usage_time.h
	gcc -Wall -g -c screen_time_daemon.c

timelimit.o: CODE/timelimit.c CODE/timelimit.h
	sudo gcc -Wall -g -c CODE/timelimit.c

usage_time.o: CODE/usage_time.c CODE/usage_time.h hashmap.c/hashmap.h
	gcc -Wall -g -c CODE/usage_time.c

hashmap.o: hashmap.c/hashmap.c hashmap.c/hashmap.h
	gcc -Wall -g -c hashmap.c/hashmap.c

left_time.log:
	echo "" > left_time.log

clean:
	$(RM) *.o *.log screen_time screen_time_daemon
