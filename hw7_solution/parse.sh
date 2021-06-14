#!/bin/bash
awk '$1==0 {print $2}' log > s0.out
awk '$1==1 {print $2}' log > s1.out
awk '$1==2 {print $2}' log > s2.out
awk '$1==3 {print $2}' log > s3.out
awk '$1==4 {print $2}' log > s4.out
