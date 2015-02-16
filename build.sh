#! /bin/bash

NUMPROC=$(cat /proc/cpuinfo | grep processor | wc -l)
#TARGETS="cracking_mt_alt_1_vectorized cracking_mt_alt_2_vectorized scanning sorted"
TARGETS=all

make clean
for vectorbits in 13; do 
    for threadbits in 1; do 
	make -j4 $TARGETS VECTORSIZE=$[2**vectorbits] THREADS=$NUMPROC EVENTS_TO_COUNT='{"UNHALTED_CORE_CYCLES"}' PIVOT=$pivot; 
    done; 
done; 

