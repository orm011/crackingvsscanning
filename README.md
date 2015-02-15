# Database Cracking: Fancy Scan, not Poor Man’s Sort! #

This is the code accompanying the DaMoN 2014 paper titled: Database Cracking: Fancy Scan, not Poor Man’s Sort!. Check out the paper using [this link](http://oai.cwi.nl/oai/asset/22474/22474D.pdf).

The code is part of a research study and **NOT MEANT FOR ANY KIND OF PRODUCTION ENVIRONMENT**.

### Requirements ###

Other than the basic tools like *make* and *gcc* you will need [PAPI](http://icl.cs.utk.edu/papi/)
to regenerate the results. Please, read the paper and the references to understand how the
profiling has been done. If something is unclear, feel free to contact holger or, even better, create a fork of this repository, update the readme and create a pull request.

### Notes ###

Note that while the cracking results are influenced very little by the
choice of the random number generator. The motivational diagram on
page one of the paper was generated using the (fast) holgerD generator
while the evaluation charts were generated using the (slower) randomD
generator. Keep this in mind when experimenting.

### Usage ###

Here is a shell script that runs the experiments for the evaluation of
the multi-threaded implementations:

    LASTCPU=$(cat /proc/cpuinfo  | grep processor | tail -n 1 | cut -d":" -f 2)
    cd /tmp
    hg clone https://holger@bitbucket.org/holger/crackingscanvssort
    cd crackingscanvssort
    hg id -bint
    make clean;
    for vectorbits in 13; do
    for threadbits in 1; do
    for pivot in 1 10 20 30 40 50 60 70 80 90 99; do
    make cracking_mt_alt_1 cracking_mt_alt_1_vectorized cracking_mt_alt_2 cracking_mt_alt_2_vectorized scanning vectorizedVanilla VECTORSIZE=$[2**vectorbits] THREADS=$[LASTCPU+1] EVENTS_TO_COUNT='{"UNHALTED_CORE_CYCLES"}' PIVOT=$pivot;
    done;
    done;
    done;
    for iteration in {1..7}; do
    for i in bin/*; do $i 4092; done;
    done;
    cd /tmp
    rm -rf pacemaker/
    
Here is the script for the single-threaded implemenation:

	LASTCPU=$(cat /proc/cpuinfo  | grep processor | tail -n 1 | cut -d":" -f 2)
	cd /tmp
    hg clone https://holger@bitbucket.org/holger/crackingscanvssort
    cd crackingscanvssort
	hg id -bint
	make clean;
	for vectorbits in 13; do
	for threadbits in 0; do
	for pivot in 1 10 20 30 40 50 60 70 80 90 99; do
	make original predicated2 predicatedInRegister scanning vectorizedVanilla VECTORSIZE=$[2**vectorbits] THREADS=$[LASTCPU+1] EVENTS_TO_COUNT='{"UNHALTED_CORE_CYCLES"}' PIVOT=$pivot;
	done;
	done;
	done;
	for iteration in {1..7}; do
	for i in bin/*; do $i 4092; done;
	done;
	cd /tmp
	rm -rf pacemaker/
