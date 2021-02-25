#!/bin/bash
sudo ./t1rm &
sudo ./t3rm &
sleep 10
sudo pkill -SIGINT t1rm
sudo pkill -SIGINT t3rm
sleep 10
sudo pkill -SIGINT t1rm
sudo pkill -SIGINT t3rm
sleep 5
sudo pkill -SIGKILL t1rm
sudo pkill -SIGKILL t3rm
