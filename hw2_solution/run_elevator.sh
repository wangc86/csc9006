#!/bin/bash

# run this with `sudo`
OUTFILE=./resp.out.100
chrt -f 99 ./high_prio_t &
./control1 2> $OUTFILE &
./drive 4
pkill high_prio_t
