#!/bin/bash

# Measure the cpu utilization for cores 0 and 1,
# with a sampling rate of 1 second; ten measurements.
mpstat -u -P 0-1 1 10 > cpu_utilization &
