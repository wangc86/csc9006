#!/bin/bash

for i in ./EDF* ./RM* ./FIFO*;
do
    echo $i
    tail -n 1 $i/cpu_utilization.out  >> $i/temp
    cat $i/temp | awk '{print (100-$12)}' > $i/cpu0
    rm $i/temp
done
