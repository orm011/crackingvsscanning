#! /bin/bash

NUMPROC=$(cat /proc/cpuinfo | grep processor | wc -l)

STTARGETS="original predicated2 predicatedInRegister scanning vectorizedVanilla"
MTTARGETS="cracking_mt_alt_1 cracking_mt_alt_1_vectorized cracking_mt_alt_2 cracking_mt_alt_2_vectorized scanning vectorizedVanilla"

make clean
for vectorbits in 13; do 
    for threadbits in 1; do 
	make -j4 $MTTARGETS VECTORSIZE=$[2**vectorbits] THREADS=$NUMPROC EVENTS_TO_COUNT='{"UNHALTED_CORE_CYCLES"}'
    done; 
done; 

