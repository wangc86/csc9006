#!/bin/bash

for i in ./EDF* ./RM* ./FIFO*;
do
    echo $i
    for j in $(seq 1 20);
    do
        tail -n 2 $i/cpu_utilization_$j.out | head -n 1 >> $i/temp0
        tail -n 1 $i/cpu_utilization_$j.out  >> $i/temp1
    done
    cat $i/temp0 | awk '{print (100-$12)}' > $i/cpu0
    cat $i/temp1 | awk '{print (100-$12)}' > $i/cpu1
    rm $i/temp0
    rm $i/temp1
done
