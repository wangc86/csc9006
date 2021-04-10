#!/bin/bash

# A script to unset the CPU frequency.

# Before use, modify related parameters
# according to your machine's hardware spec.
for i in $(seq 0 7);
do
    sudo cpufreq-set -d 400000 -c $i -r
    sudo cpufreq-set -u 4600000 -c $i -r
done

sleep 5

for i in $(seq 0 7);
do
    sudo cpufreq-set -g powersave -c $i -r
done
# Type 'cpufreq-info' to verify
# the changes you've made.
