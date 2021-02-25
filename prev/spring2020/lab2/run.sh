#!/bin/bash


if [ -z "$1" ]
then
    echo "Usage: $0 (the # of repetitions)"
    exit
fi

# cs3pcp is the name of my executable binary

exe=cs3pcp
for i in $(seq 1 $1);
do
    echo "Starting test #$i .."
    sudo ./$exe $i &
    sleep 10
    echo "Killing test #$i .."
    sudo pkill -9 $exe
    sleep 2
done
