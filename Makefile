ifeq ($(shell uname),Darwin)
CFLAGS=-DNO_PAPI -O3 -march=native -mtune=native -g
LDFLAGS=
else
# EVENTS_TO_COUNT={"CYCLE_ACTIVITY:STALLS_L2_PENDING", "CYCLE_ACTIVITY:STALLS_L1D_PENDING", "UNHALTED_CORE_CYCLES"}
ifeq (,$(findstring bricks,$(shell hostname)))
EVENTS_TO_COUNT={"UNHALTED_CORE_CYCLES"}
else
EVENTS_TO_COUNT={"UNHALTED_CORE_CYCLES"}
endif
CFLAGS=-O3 -march=native -mtune=native -ftree-vectorize -funroll-loops -fsched-spec-load  -falign-loops -floop-parallelize-all -faggressive-loop-optimizations -g -DEVENTS_TO_COUNT='$(EVENTS_TO_COUNT)' -fopenmp 
# CFLAGS=-O0 -g -march=native -mtune=native 
LDFLAGS=-lm
endif
VECTORSIZE=1024
THREADS=4
#percentage of the value domain(PIVOT: 0%-100%)
PIVOT=50
#possible values: randomD,uniformD,skewedD,holgerD,sortedD,revsortedD,almostsortedD
DISTRIBUTION=randomD
SEED=100003
SKEW=10
ifeq (,$(findstring bricks,$(shell hostname)))
CFLAGS+=-DNO_PAPI
else
LDFLAGS+=-lpapi
endif

all: original naive bandwidth vectorizedVanilla vectorizedWithAVXMemcpy vectorizedWithAVXMemcpyAndSIMDCracking cracking_mt_alt_1 cracking_mt_alt_2 cracking_mt_alt_1_vectorized cracking_mt_alt_2_vectorized cracking_mt_alt_1_notmerge cracking_mt_alt_2_notmerge cracking scanning sorting predicated simd

naive: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/naive_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/naive.c Framework/main.c Implementations/distributions.c Implementations/create_values.c

bandwidth: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -fopenmp -std=gnu99 -o bin/bandwidth_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/bandwidth.c Framework/main.c Implementations/distributions.c Implementations/create_values.c

vectorizedVanilla: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/vectorized$(VECTORSIZE)Ints_$(DISTRIBUTION)_$(PIVOT) -DVECTORSIZE=$(VECTORSIZE) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorized.c Framework/main.c Implementations/distributions.c Implementations/create_values.c

vectorizedWithAVXMemcpy: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/vectorizedWithAVXMemcpy$(VECTORSIZE)Ints_$(DISTRIBUTION)_$(PIVOT) -DVECTORSIZE=$(VECTORSIZE) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpy.c Framework/main.c Implementations/distributions.c Implementations/create_values.c

vectorizedWithAVXMemcpyAndSIMDCracking: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/vectorizedWithAVXMemcpyAndSIMDCracking$(VECTORSIZE)Ints_$(DISTRIBUTION)_$(PIVOT) -DVECTORSIZE=$(VECTORSIZE) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpyAndSIMDCracking.c Framework/main.c Implementations/distributions.c Implementations/create_values.c

original: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/original_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c Implementations/threadpool.c Framework/main.c Implementations/original.c Implementations/distributions.c Implementations/create_values.c

cracking_mt_alt_1: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_$(DISTRIBUTION)_$(PIVOT)_alt_1_threads_$(THREADS) -DNTHREADS=$(THREADS) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_1.c Implementations/distributions.c Implementations/create_values.c

cracking_mt_alt_2: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_$(DISTRIBUTION)_$(PIVOT)_alt_2_threads_$(THREADS) -DNTHREADS=$(THREADS) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_2.c Implementations/distributions.c Implementations/create_values.c

cracking_mt_alt_1_vectorized: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_$(DISTRIBUTION)_$(PIVOT)_alt_1_$(VECTORSIZE)_int_vectors_threads_$(THREADS) -DVECTORSIZE=$(VECTORSIZE) -DNTHREADS=$(THREADS) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_vectorized.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_1_vectorized.c Implementations/distributions.c Implementations/create_values.c

cracking_mt_alt_2_vectorized: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_$(DISTRIBUTION)_$(PIVOT)_alt_2_vectorized_threads_$(THREADS) -DVECTORSIZE=$(VECTORSIZE) -DNTHREADS=$(THREADS) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_vectorized.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_2_vectorized.c Implementations/distributions.c Implementations/create_values.c

cracking_mt_alt_1_notmerge: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt__$(DISTRIBUTION)_$(PIVOT)alt_1_notmerge_threads_$(THREADS) -DNTHREADS=$(THREADS) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_notmerge.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_1_notmerge.c Implementations/distributions.c Implementations/create_values.c

cracking_mt_alt_2_notmerge: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt__$(DISTRIBUTION)_$(PIVOT)alt_2_notmerge_threads_$(THREADS) -DNTHREADS=$(THREADS) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_notmerge.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_2_notmerge.c Implementations/distributions.c Implementations/create_values.c

outputdir:
	mkdir -v bin 2>/dev/null || true

asmdir:
	mkdir -v asm 2>/dev/null || true

clean:
	rm -rf bin

asm/vectorized.asm: asmdir
	gcc -S $(LDFLAGS) $(CFLAGS) -std=gnu99 -o asm/vectorized_$(DISTRIBUTION)_$(PIVOT).asm -DVECTORSIZE=4096 -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpyAndSIMDCracking.c Implementations/distributions.c Implementations/create_values.c

asm/bandwidth.asm: asmdir
	gcc -S $(LDFLAGS) $(CFLAGS) -std=gnu99 -o asm/bandwidth_$(DISTRIBUTION)_$(PIVOT).asm -DVECTORSIZE=4096 -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/bandwidth.c Implementations/distributions.c Implementations/create_values.c

cracking: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/Cracking_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c Implementations/threadpool.c Framework/main.c Implementations/original.c Implementations/distributions.c Implementations/create_values.c

scanning: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -fopenmp -std=gnu99 -o bin/Scanning_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/bandwidth.c Framework/main.c Implementations/distributions.c Implementations/create_values.c

sorting: outputdir
	g++ $(LDFLAGS) $(CFLAGS) -fopenmp -o bin/Sorting_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/sort.c Framework/main.c  Implementations/distributions.c Implementations/create_values.c

predicated: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicated_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) -DPIVOT=$(PIVOT) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicated.c 

predicated2: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicated2_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) -DPIVOT=$(PIVOT) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicated2.c 

predicatedInRegister: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicatedInRegister_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) -DPIVOT=$(PIVOT) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicatedInRegister.c 

predicatedInRegister2: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicatedInRegister2_$(DISTRIBUTION)_$(PIVOT) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) -DPIVOT=$(PIVOT) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicatedInRegister2.c 


simd: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/simd$(VECTORSIZE)Ints_$(DISTRIBUTION)_$(PIVOT) -DVECTORSIZE=$(VECTORSIZE) -DPIVOT=$(PIVOT) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/simd.c Framework/main.c Implementations/distributions.c Implementations/create_values.c