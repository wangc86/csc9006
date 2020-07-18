#!/bin/bash

BROKER=es_server
SUB=es_sub
PUB=es_pub

sudo pkill -9 $PUB
sudo pkill -9 $SUB
sudo pkill -9 $BROKER
