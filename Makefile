# EVENTS_TO_COUNT={"CYCLE_ACTIVITY:STALLS_L2_PENDING", "CYCLE_ACTIVITY:STALLS_L1D_PENDING", "UNHALTED_CORE_CYCLES"}
EVENTS_TO_COUNT={"UNHALTED_CORE_CYCLES"}

OUTFLAGS=-O3 -ftree-vectorize -funroll-loops -fsched-spec-load -falign-loops -DEVENTS_TO_COUNT='$(EVENTS_TO_COUNT)'
PAPI=-DNO_PAPI

#-faggressive-loop-optimizations <- not available in istc*
#-floop-parallelize-all <- needs config'd gcc

VECTORSIZE=1024
TIMING=0 #for extended gettimeof day profiling

#possible values: randomD,uniformD,skewedD,holgerD,sortedD,revsortedD,almostsortedD
DISTRIBUTION=randomD
SEED=100003
SKEW=10
CFLAGSNOPEN=$(OUTFLAGS) $(PAPI) -march=native -mtune=native -fno-omit-frame-pointer -g
CFLAGS=$(OUTFLAGS) $(PAPI) -march=native -mtune=native -fopenmp -fno-omit-frame-pointer -g

RONLY=1 #scanning (bandwidth.c) is read only, affects nothing else.
AFFINITY=0

TASKS_PER_THREAD=4

PCMON=0

COMMOND=-DAFFINITY=$(AFFINITY) -DPCMON=$(PCMON) -DTIMING=$(TIMING) -DSEED=$(SEED) -DSKEW=$(SKEW) -DVECTORSIZE=$(VECTORSIZE) -DNTHREADS=$(THREADS) -DDISTRIBUTION=$(DISTRIBUTION) -DTASKS_PER_THREAD=$(TASKS_PER_THREAD)
COMMONF=Framework/main.c Implementations/distributions.c Implementations/create_values.c
COMMON=$(COMMOND) $(COMMONF) 

THREADS:=$(shell cat /proc/cpuinfo | grep processor | wc -l)
LDFLAGS=-lm -lpthread

HELPER=-pthread -fpermissive $(CFLAGS) $(COMMOND) $(LDFLAGS)
PCM=/home/orm/IntelPerformanceCounterMonitorV2.8/

all: scanning cracking_mt_alt_2_vectorized

cracking_mt_alt_2_vectorized: outputdir
ifeq ($(PCMON),1)
	g++ -fpermissive -I $(PCM) -pthread $(CFLAGS) -std=c++0x -o bin/cracking_mt_alt_2_vectorized  Implementations/cracking_MT_vectorized.c  Implementations/threadpool.c Implementations/cracking_mt_alt_2_vectorized.c  $(COMMON) $(LDFLAGS) -L$ $(PCM)/intelpcm.so/ -lintelpcm
else
	g++ -fpermissive -pthread $(CFLAGS) --std=c++0x -o bin/cracking_mt_alt_2_vectorized  Implementations/cracking_MT_vectorized.c  Implementations/threadpool.c Implementations/cracking_mt_alt_2_vectorized.c  $(COMMON) $(LDFLAGS)		
endif

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

outputdir:
	mkdir -v bin 2>/dev/null || true

asmdir:
	mkdir -v asm 2>/dev/null || true

clean:
	rm -rf bin

asm/vectorized.asm: asmdir
	gcc -S $(LDFLAGS) $(CFLAGS) -std=gnu99 -o asm/vectorized_$(DISTRIBUTION).asm -DVECTORSIZE=4096 -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/vectorizedWithAVXMemcpyAndSIMDCracking.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

asm/scanning.asm: asmdir
	gcc -S $(LDFLAGS) $(CFLAGS) -std=gnu99 -o asm/bandwidth_$(DISTRIBUTION).asm -DVECTORSIZE=4096 -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/bandwidth.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

cracking: outputdir
	gcc -pthread $(LDFLAGS) $(CFLAGS) -std=gnu99 -o bin/Cracking_$(DISTRIBUTION) -DDISTRIBUTION=$(DISTRIBUTION) -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/cracking_MT.c Implementations/threadpool.c Framework/main.c Implementations/original.c Implementations/distributions.c Implementations/create_values.c $(LDFLAGS)

scanning: outputdir
ifeq ($(PCMON),1)
	g++ -fpermissive -I $(PCM)  $(CFLAGS) -std=c++0x -DRONLY=1 -o ./bin/scanning Implementations/bandwidth.c  $(COMMON) $(LDFLAGS) -L$ $(PCM)/intelpcm.so/ -lintelpcm
else
	gcc $(CFLAGS) -fopenmp -std=gnu99 -o bin/scanning -DRONLY=1 Implementations/bandwidth.c $(COMMON) $(LDFLAGS) 
endif

copying: outputdir
	gcc $(CFLAGS) -fopenmp -std=gnu99 -o bin/copying -DRONLY=0 -DSEED=$(SEED) -DSKEW=$(SKEW) Implementations/bandwidth.c $(COMMON) $(LDFLAGS) 

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
