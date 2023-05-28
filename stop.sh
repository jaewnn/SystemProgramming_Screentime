#!/bin/bash

result=$(ps aux | grep screen_time_daemon)
pid=$(echo $result | cut -d " " -f 2)
sudo -v
sudo kill $pid
