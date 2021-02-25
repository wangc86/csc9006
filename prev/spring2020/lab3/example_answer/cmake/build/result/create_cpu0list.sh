#!/bin/bash

for n in 20 40 60 80 100;
do
    for i in ./FIFO/FIFO-config$n*;
    do
        cat $i/cpu0  >> ./FIFO/cpu0list-FIFO-config$n
    done
done
