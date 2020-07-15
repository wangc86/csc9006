#!/bin/bash

BROKER=es_server
SUB=es_sub
PUB=es_pub
for policy in RM;
do
    #for load in $(seq 1500 3000 13501);
    for conf in config;
    do
        # create a folder to store result of this config
        mkdir -p $policy-$conf
        # repeat the test 20 times
        #for i in $(seq 1 20);
        for i in 1;
        do
            echo "----- Current Run --------"
            echo "| Policy: $policy "
            echo "| Config file: $conf "
            echo "| Round #$i "
            echo "--------------------------"

            echo "Starting our broker .."
            #echo "sudo ./$BROKER -c $conf -s $policy &"
            sudo ./$BROKER -c $conf -s $policy &

            # allow a few seconds before we move on
            sleep 2

            echo "Starting our sub .."
            #echo "sudo ./$SUB -t 0 &" 
            sudo ./$SUB -t 0 > i0 & 
            sudo ./$SUB -t 1 > i1 & 
            sudo ./$SUB -t 2 > i2 & 

            # allow a few seconds before we move on
            sleep 2

            echo "Starting our pub .."
       #     echo "sudo ./$PUB -l $load &"
            sudo ./$PUB -c $conf -i 0 & 
            sudo ./$PUB -c $conf -i 1 & 
            sudo ./$PUB -c $conf -i 2 & 

            # allow 30 seconds to warm up
            #sleep 30
            sleep 15

            # start measure CPU utilization for two minutes
       #     mpstat -u -P 0-1 1 120 > cpu_utilization_$i.out &
       #     mpstat -u -P 0-1 1 1 > cpu_utilization_$i.out &

            # collecting experimental data
       #     sleep 120

            # allow a few seconds before we move on
            sleep 2

            echo "Killing test #$i .."
            sudo pkill -9 $PUB
            sudo pkill -9 $SUB
            sudo pkill -9 $BROKER
            sleep 1

            # move the results to the specified folder
       #     mv *.out ./$policy-$conf
            mv i0 ./$policy-$conf
            mv i1 ./$policy-$conf
            mv i2 ./$policy-$conf
        done
    done
done
