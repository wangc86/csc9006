#!/bin/bash
sudo ./response &
sleep 12
sudo pkill -SIGINT response
sleep 5
sudo pkill -SIGKILL response
