#!/bin/bash

BROKER=yourBrokerName
SUB=yourSubName
PUB=yourPubName
for policy in EDF RM FIFO;
do
    for load in $(seq 1500 3000 13501);
    do
        # create a folder to store result of this config
        mkdir $policy-$load
        # repeat the test 20 times
        for i in $(seq 1 20);
     #   for i in $(seq 1 10);
        do
            echo "----- Current Run --------"
            echo "| Policy: $policy "
            echo "| Load: $load "
            echo "| Round #$i "
            echo "--------------------------"

            echo "Starting our broker .."
       #     echo "sudo ./$BROKER -c config.txt -s $policy -n $i &"
       #     sudo ./$BROKER -c config.txt -s $policy -n $i &

            # allow a few seconds before we move on
            sleep 2

            echo "Starting our sub .."
       #     echo "sudo ./$SUB &" 
       #     sudo ./$SUB & 

            # allow a few seconds before we move on
            sleep 2

            echo "Starting our pub .."
       #     echo "sudo ./$PUB -l $load &"
       #     sudo ./$PUB -l $load & 

            # allow 30 seconds to warm up
            sleep 30

            # start measure CPU utilization for two minutes
            mpstat -u -P 0-1 1 120 > cpu_utilization_$i.out &
       #     mpstat -u -P 0-1 1 1 > cpu_utilization_$i.out &

            # collecting experimental data
            sleep 120
       #     sleep 1

            # allow a few seconds before we move on
            sleep 2

            echo "Killing test #$i .."
       #     sudo pkill -9 $PUB
       #     sudo pkill -9 $SUB
       #     sudo pkill -9 $BROKER
            sleep 1

            # move the results to the specified folder
            mv *.out ./$policy-$load
        done
    done
done
