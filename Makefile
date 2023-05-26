.PHONY: all screen_time screen_time_daemon clean

all:
	make screen_time
	make screen_time_daemon
	make left_time.log
	make sample_data
	make execute_remove.log

screen_time: screen_time.o usage_time.o hashmap.o
	gcc -o screen_time screen_time.o usage_time.o hashmap.o -lcurses -lrt

screen_time_daemon: screen_time_daemon.o usage_time.o hashmap.o
	gcc -o screen_time_daemon screen_time_daemon.o usage_time.o hashmap.o -lrt

screen_time.o: screen_time.c CODE/usage_time.h
	gcc -Wall -g -c screen_time.c

screen_time_daemon.o: screen_time_daemon.c CODE/usage_time.h
	gcc -Wall -g -c screen_time_daemon.c

usage_time.o: CODE/usage_time.c CODE/usage_time.h hashmap.c/hashmap.h
	gcc -Wall -g -c CODE/usage_time.c

hashmap.o: hashmap.c/hashmap.c hashmap.c/hashmap.h
	gcc -Wall -g -c hashmap.c/hashmap.c

left_time.log:
	echo "" > left_time.log

execute_remove.log:
	echo "" > execute_remove.log

sample_data:
	echo "sample 10000" > usage_time_0.log
	echo "sample 20000" > usage_time_1.log
	echo "sample 30000" > usage_time_2.log
	echo "sample 40000" > usage_time_3.log
	echo "sample 50000" > usage_time_4.log
	echo "sample 60000" > usage_time_5.log
	echo "sample 70000" > usage_time_6.log
	make exclude_process.log

exclude_process.log:
	echo "gnome-session-c 0" >> exclude_process.log
	echo "gsd-color 0" >> exclude_process.log
	echo "at-spi-bus-laun 0" >> exclude_process.log
	echo "evolution-addre 0" >> exclude_process.log
	echo "ibus-extension- 0" >> exclude_process.log
	echo "gsd-print-notif 0" >> exclude_process.log
	echo "gnome-keyring-d 0" >> exclude_process.log
	echo "goa-identity-se 0" >> exclude_process.log
	echo "gvfsd-metadata 0" >> exclude_process.log
	echo "(sd-pam) 0" >> exclude_process.log
	echo "gsd-sharing 0" >> exclude_process.log
	echo "gvfs-afc-volume 0" >> exclude_process.log
	echo "ibus-engine-sim 0" >> exclude_process.log
	echo "gsd-disk-utilit 0" >> exclude_process.log
	echo "gsd-xsettings 0" >> exclude_process.log
	echo "gnome-terminal- 0" >> exclude_process.log
	echo "gsd-datetime 0" >> exclude_process.log
	echo "pulseaudio 0" >> exclude_process.log
	echo "gsd-wwan 0" >> exclude_process.log
	echo "gvfsd-fuse 0" >> exclude_process.log
	echo "ibus-daemon 0" >> exclude_process.log
	echo "desktopControll 0" >> exclude_process.log
	echo "ibus-portal 0" >> exclude_process.log
	echo "evolution-sourc 0" >> exclude_process.log
	echo "gsd-media-keys 0" >> exclude_process.log
	echo "gsd-keyboard 0" >> exclude_process.log
	echo "ibus-engine-han 0" >> exclude_process.log
	echo "gsd-housekeepin 0" >> exclude_process.log
	echo "nautilus 0" >> exclude_process.log
	echo "gnome-shell-cal 0" >> exclude_process.log
	echo "ssh-agent 0" >> exclude_process.log
	echo "gsd-power 0" >> exclude_process.log
	echo "evolution-alarm 0" >> exclude_process.log
	echo "gsd-usb-protect 0" >> exclude_process.log
	echo "xdg-permission- 0" >> exclude_process.log
	echo "dconf-service 0" >> exclude_process.log
	echo "dbus-daemon 0" >> exclude_process.log
	echo "gvfsd 0" >> exclude_process.log
	echo "gsd-sound 0" >> exclude_process.log
	echo "at-spi2-registr 0" >> exclude_process.log
	echo "gnome-shell 0" >> exclude_process.log
	echo "systemd 0" >> exclude_process.log
	echo "gnome-session-b 0" >> exclude_process.log
	echo "gvfsd-trash 0" >> exclude_process.log
	echo "gsd-screensaver 0" >> exclude_process.log
	echo "gsd-a11y-settin 0" >> exclude_process.log
	echo "goa-daemon 0" >> exclude_process.log
	echo "vmtoolsd 0" >> exclude_process.log
	echo "gsd-rfkill 0" >> exclude_process.log
	echo "ibus-x11 0" >> exclude_process.log
	echo "gvfs-udisks2-vo 0" >> exclude_process.log
	echo "gsd-wacom 0" >> exclude_process.log
	echo "gsd-printer 0" >> exclude_process.log
	echo "ibus-dconf 0" >> exclude_process.log
	echo "tracker-miner-f 0" >> exclude_process.log
	echo "gjs 0" >> exclude_process.log
	echo "xdg-desktop-por 0" >> exclude_process.log
	echo "gvfs-mtp-volume 0" >> exclude_process.log
	echo "gsd-smartcard 0" >> exclude_process.log
	echo "xdg-document-po 0" >> exclude_process.log
	echo "evolution-calen 0" >> exclude_process.log
	echo "update-notifier 0" >> exclude_process.log
	echo "gvfs-gphoto2-vo 0" >> exclude_process.log
	echo "screen_time_dae 0" >> exclude_process.log
	echo "gvfs-goa-volume 0" >> exclude_process.log
	echo "bash 0" >> exclude_process.log

clean:
	$(RM) *.o *.log screen_time screen_time_daemon
