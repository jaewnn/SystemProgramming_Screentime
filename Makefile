.PHONY: all log clean

all:
	mkdir -p obj
	make screen_time
	make screen_time_daemon
	make log

log:
	$(MAKE) -C log

clean:
	$(RM) obj/* screen_time screen_time_daemon
	$(MAKE) clean -C log

screen_time: screen_time.o screen_time_func.o usage_time.o top.o hashmap.o
	gcc -o screen_time obj/screen_time.o obj/screen_time_func.o obj/top.o obj/usage_time.o obj/hashmap.o -lrt -lcurses

screen_time_daemon: screen_time_daemon.o usage_time.o hashmap.o
	gcc -o screen_time_daemon obj/screen_time_daemon.o obj/usage_time.o obj/hashmap.o -lrt

screen_time.o: src/screen_time.c src/screen_time_func.h
	gcc -Wall -g -c src/screen_time.c -o obj/screen_time.o

screen_time_func.o: src/screen_time_func.c src/usage_time.h src/top.h
	gcc -Wall -g -c src/screen_time_func.c -o obj/screen_time_func.o

screen_time_daemon.o: src/screen_time_daemon.c src/usage_time.h
	gcc -Wall -g -c src/screen_time_daemon.c -o obj/screen_time_daemon.o

usage_time.o: src/usage_time.c src/usage_time.h hashmap.c/hashmap.h
	gcc -Wall -g -c src/usage_time.c -o obj/usage_time.o

top.o: src/top.c src/top.h
	gcc -Wall -g -c src/top.c -o obj/top.o

hashmap.o: hashmap.c/hashmap.c hashmap.c/hashmap.h
	gcc -Wall -g -c hashmap.c/hashmap.c -o obj/hashmap.o
