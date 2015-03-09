# EVENTS_TO_COUNT={"CYCLE_ACTIVITY:STALLS_L2_PENDING", "CYCLE_ACTIVITY:STALLS_L1D_PENDING", "UNHALTED_CORE_CYCLES"}

NPROCS:=$(shell cat /proc/cpuinfo | grep processor | wc -l)
THREADS=$(NPROCS)

CC=g++
CCFLAGS=-g -std=c++0x -fpermissive -ftree-vectorize -funroll-loops -fno-omit-frame-pointer -march=native -mtune=native

LDFLAGS=-lm -lpthread

ifeq ($(CC),clang++)
CCFLAGS+=-fcilkplus
LDFLAGS+=-ldl -l:libcilkrts.a
endif

ifeq ($(PCMON), 1)
LDFLAGS+=-L $(PCM)/intelpcm.so/ -lintelpcm -Wl,-rpath $(PCM)/intelpcm.so/
endif

ifeq ($(CC),clang++)
endif

ifndef NOMP
CCFLAGS+=-fopenmp
endif

ifdef DEBUG
CCFLAGS+=-O1
else
CCFLAGS+=-O3
endif

#-fsched-spec-load -falign-loops <- not available on my clang branch., but also not showing consisent perf.
#-faggressive-loop-optimizations <- not available in istc*
#-floop-parallelize-all <- needs config'd gcc

VECTORSIZE=1024
TIMING=0 #for extended gettimeof day profiling
TASKS_PER_THREAD=1
DISTRIBUTION=randomD
SEED=100003
SKEW=10

AFFINITY=0
PCMON=0
COARSENING=1024 

COMMONDEFS=-DAFFINITY=$(AFFINITY) -DCOARSENING=$(COARSENING) -DNO_PAPI -DPCMON=$(PCMON) -DTIMING=$(TIMING) -DSEED=$(SEED) -DSKEW=$(SKEW) -DVECTORSIZE=$(VECTORSIZE) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) -DTASKS_PER_THREAD=$(TASKS_PER_THREAD)

COMMONFILES=Framework/main.c Implementations/distributions.c Implementations/create_values.c
COMMON=$(COMMONDEFS) $(COMMONFILES) 

HOME:=$(shell echo ~)
PCM=$(HOME)/IntelPerformanceCounterMonitorV2.8/

IFLAGS=
ifeq ($(PCMON), 1)
IFLAGS+=-I $(PCM)
endif


all: scanning cracking copying cilkpivot

outputdir:
	mkdir -v bin 2>/dev/null || true

asmdir:
	mkdir -v asm 2>/dev/null || true

clean:
	rm -rf bin

cracking: outputdir
	$(CC) $(CCFLAGS) $(IFLAGS) -o bin/cracking  Implementations/cracking_MT_vectorized.c  Implementations/threadpool.c Implementations/cracking_mt_alt_2_vectorized.c  $(COMMON) $(LDFLAGS) 

scanning: outputdir
	$(CC) $(CCFLAGS) $(IFLAGS) -o ./bin/scanning Implementations/scanning.c $(COMMON) $(LDFLAGS)

cilkpivot: outputdir
	$(CC) $(CCFLAGS) $(IFLAGS) -o ./bin/cilkpivot Implementations/cilkpivot.cc $(COMMON) $(LDFLAGS)

scanning.asm: asmdir
	$(CC) -S $(CCFLAGS) $(IFLAGS) -o ./asm/scanning.asm Implementations/scanning.c 

copying: outputdir
	$(CC) $(CCFLAGS) $(IFLAGS) -o ./bin/copying Implementations/copying.cc $(COMMON) $(LDFLAGS) 

copying.asm: asmdir
	$(CC) -S $(CCFLAGS) $(IFLAGS) -o ./asm/copying.asm Implementations/copying.cc

naive: outputdir
	gcc  $(CFLAGS) -std=gnu99 -o bin/naive_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/naive.c Framework/main.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

vectorizedVanilla: outputdir
	gcc  $(CFLAGS) -std=gnu99 -o bin/vectorized$(VECTORSIZE)Ints_$(DISTRIBUTION) -DVECTORSIZE=$(VECTORSIZE) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorized.c Framework/main.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

vectorizedWithAVXMemcpy: outputdir
	gcc  $(CFLAGS) -std=gnu99 -o bin/vectorizedWithAVXMemcpy$(VECTORSIZE)Ints_$(DISTRIBUTION) -DVECTORSIZE=$(VECTORSIZE) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpy.c Framework/main.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

vectorizedWithAVXMemcpyAndSIMDCracking: outputdir
	gcc  $(CFLAGS) -std=gnu99 -o bin/vectorizedWithAVXMemcpyAndSIMDCracking$(VECTORSIZE)Ints_$(DISTRIBUTION) -DVECTORSIZE=$(VECTORSIZE) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpyAndSIMDCracking.c Framework/main.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

original: outputdir
	gcc -pthread  $(CFLAGS) -std=gnu99 -o bin/original_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c Implementations/threadpool.c Framework/main.c Implementations/original.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

cracking_mt_alt_1: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_alt_1_threads_$(THREADS) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_1.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

cracking_mt_alt_2: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_alt_2_threads_$(THREADS) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_2.c $(COMMON) $(LDFLAGS)

cracking_mt_alt_1_vectorized: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_alt_1_$(VECTORSIZE)_int_vectors_threads_$(THREADS) -DVECTORSIZE=$(VECTORSIZE) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_vectorized.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_1_vectorized.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

cracking_mt_alt_1_notmerge: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_alt_1_notmerge_threads_$(THREADS) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_notmerge.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_1_notmerge.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

cracking_mt_alt_2_notmerge: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/cracking_mt_alt_2_notmerge_threads_$(THREADS) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT_notmerge.c  Implementations/threadpool.c   Framework/main.c Implementations/cracking_mt_alt_2_notmerge.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)


asm/vectorized.asm: asmdir
	gcc -S $(LDFLAGS) $(CFLAGS) -std=gnu99 -o asm/vectorized_$(DISTRIBUTION).asm -DVECTORSIZE=4096 -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpyAndSIMDCracking.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

asm/scanning.asm: asmdir
	gcc -S $(LDFLAGS) $(CFLAGS) -std=gnu99 -o asm/bandwidth_$(DISTRIBUTION).asm -DVECTORSIZE=4096 -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/bandwidth.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

cracking_old: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/Cracking_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c Implementations/threadpool.c Framework/main.c Implementations/original.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)


sorting: outputdir
	g++ $(LDFLAGS) $(CFLAGS) -fopenmp -o bin/sorting_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/sort.c Framework/main.c  Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

stdpartition: outputdir
	g++ $(LDFLAGS) $(CFLAGS) -fopenmp -o bin/stdpartition_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/stdpartition.cc $(COMMON) $(LDFLAGS)

predicated: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicated_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicated.c  $(LDFLAGS)

predicated2: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicated2_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicated2.c  $(LDFLAGS)

predicatedInRegister: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicatedInRegister_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicatedInRegister.c  $(LDFLAGS)

predicatedInRegister2: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/predicatedInRegister2_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Framework/main.c Implementations/distributions.c Implementations/create_values.c Implementations/predicatedInRegister2.c $(LDFLAGS)


simd: outputdir
	gcc $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/simd$(VECTORSIZE)Ints_$(DISTRIBUTION) -DVECTORSIZE=$(VECTORSIZE) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/simd.c Framework/main.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

memcpybench: outputdir
	g++ $(CFLAGS) -std=c++0x memcpybench.cc -o bin/memcpybench
