#!/bin/bash
sudo ./t1rmi &
sudo ./t3rmi &
sleep 10
sudo pkill -SIGINT t1rmi
sudo pkill -SIGINT t3rmi
sleep 10
sudo pkill -SIGINT t1rmi
sudo pkill -SIGINT t3rmi
sleep 5
sudo pkill -SIGKILL t1rmi
sudo pkill -SIGKILL t3rmi
