#include <stdio.h>
#include "interface.h"
#ifndef NO_PAPI
#include "papi.h"

#endif
#include <sys/time.h>
#include <getopt.h>
#include <stdint.h>
#include <assert.h>

#if PCMON == 1
#include <cpucounters.h>
#endif


long timediff(struct timeval before, struct timeval after){
	return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}

const int LLC_MISSES = 0x40000022;
const int L2_STALLS = 0x400003ac;
const int L1_STALLS = 0x400003ad;

#ifndef EVENTS_TO_COUNT
#define EVENTS_TO_COUNT {"LLC_MISSES", "CYCLE_ACTIVITY:STALLS_L2_PENDING", "CYCLE_ACTIVITY:STALLS_L1D_PENDING", "UNHALTED_CORE_CYCLES"}
#endif

#ifdef DO_PAYLOAD_SHUFFLE
const int SHUFFLE_PAYLOAD = 1;
#else
const int SHUFFLE_PAYLOAD = 0;
#endif

typedef struct features {
	uint64_t sum;
	uint64_t gepivot;
	int partitioned;
} features_t;

features_t get_features(targetType *buf, size_t len, targetType pivot) {
	features_t ans  = { 0, 0, 1 };

	int pivot_crossed = 0;
	assert(ans.partitioned);

	for (size_t i = 0; i < len ; i++) {
		pivot_crossed = pivot_crossed || buf[i] >= pivot;
		ans.partitioned = ans.partitioned && !(buf[i] < pivot && pivot_crossed);
		ans.gepivot += (buf[i] >= pivot); //update gepivot after.
		ans.sum += buf[i];
	}

	return ans;
}

void sanity_check(){
	unsigned int b[] = {0,3,1,4,5};
	size_t elts = sizeof(b)/sizeof(unsigned int);

	features_t t1 = get_features(b, elts, 0);
	assert(t1.partitioned);
	assert(t1.gepivot == 5);
	assert(t1.sum == 13);

	features_t t2 = get_features(b, elts, 2);
	assert(!t2.partitioned);
	assert(t2.gepivot == 3);

	features_t t4 = get_features(b, elts, 4);
	assert(t4.partitioned);
	assert(t4.gepivot == 2);
}

int main(int argc, char* argv[]) {
	sanity_check();
	int pivotrel = -1;
	long long int memorySize = -1;
	const char * distribution = "randomD";

    static struct option long_options[] =
      {
        /* These options set a flag. */
        {"pivot", required_argument, 0, 'p'},
        {"sizemb", required_argument, 0, 's'},
        {0, 0, 0, 0}
      };

    while (1) {
    	int * option_index = 0;
    	int c = getopt_long(argc, argv, "p:s:", long_options, option_index);

    	if (c == -1) break;

    	switch (c) {
    	case 'p':
    		pivotrel = atoi(optarg);
    		break;
    	case 's':
    		memorySize = atoi(optarg);
    		break;
    	case '?':
    		printf("arg error ?\n");
    		exit(1);
    	default:
    		printf("arg error default\n");
    		exit(1);
    	}
    }
    // fprintf(stderr, "command: %s, pivot: %d, sizemb: %d\n", argv[0], pivotrel, memorySize);
	assert(pivotrel>=0 && pivotrel<=100);
	assert(memorySize > 0);
	//assert(sizemb/sizeof(targetType) < )

	const size_t valueCount = memorySize * 1024 * 1024 / sizeof(targetType);
	targetType* buffer;

	int r = posix_memalign((void**)(&buffer), 32, valueCount * sizeof(targetType)); // TODO: fiddle with alignment
	assert(r == 0);
	payloadType* payloadBuffer = (SHUFFLE_PAYLOAD)?(payloadType*) malloc(valueCount * sizeof(payloadType)):NULL;
	unsigned long long sum_before=0, sum_after=0, sum_prod_val_pos_before=0, sum_prod_val_pos_after=0;

	targetType pivot = ((RAND_MAX/100) * pivotrel);

	create_values(distribution, buffer, valueCount, valueCount);
	features_t ftbefore = get_features(buffer, valueCount, pivot);


#ifndef NO_PAPI
	const char* events[] = EVENTS_TO_COUNT;

	int retval, EventSet=PAPI_NULL;
	long_long values[sizeof(events)/sizeof(events[0])];
	/* Initialize the PAPI library */
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT) {
  		fprintf(stderr, "PAPI library init error!\n");
  		exit(1);
	}
	/* Create the Event Set */
	if (PAPI_create_eventset(&EventSet) != PAPI_OK){
		fprintf(stderr, "PAPI failed to create event set: %s\n", PAPI_strerror(retval));
		exit(1);
	}
	if ( ( retval = PAPI_assign_eventset_component( EventSet, 0 ) ) != PAPI_OK )
		printf("PAPI_assign_eventset_component: %d", retval );
	{
		PAPI_option_t opt;
		memset( &opt, 0x0, sizeof ( PAPI_option_t ) );
		opt.inherit.inherit = PAPI_INHERIT_ALL;
		opt.inherit.eventset = EventSet;
		if ( ( retval = PAPI_set_opt( PAPI_INHERIT, &opt ) ) != PAPI_OK ) {
			if ( retval == PAPI_ECMP) {
				printf("Inherit not supported by current component: %d.\n", retval );
			} else {
				printf("PAPI_set_opt: %d", retval );
			}
		}
	}	
	for (int i = 0; i < sizeof(events)/sizeof(events[0]); i++)	{
		int event;
		char eventName[1024];
		strncpy(eventName, events[i], 1024);
		PAPI_event_name_to_code(eventName, &event);
		if ((retval = PAPI_add_event(EventSet, event)) != PAPI_OK){
			fprintf(stderr, "PAPI failed to add event %d: %s\n", i, PAPI_strerror(retval));
			exit(1);
		}
	}

	/* Start counting events in the Event Set */
	if (PAPI_start(EventSet) != PAPI_OK){
		fprintf(stderr, "PAPI cannot start counting events: %s\n", PAPI_strerror(retval));
		exit(1);
	}

#endif
	struct timeval before, after;

#if PCMON == 1
	PCM * m = PCM::getInstance();

	PCM::ErrorCode e =  m->program (PCM::DEFAULT_EVENTS, NULL);
	SystemCounterState before_sstate = m->getSystemCounterState();

	if (e != PCM::Success) {
		fprintf(stderr, "PCM::program() failed with error %d\n", e);
		abort();
	}
#endif

	gettimeofday(&before, NULL);

	performCrack(buffer, payloadBuffer, valueCount, pivot, pivotrel);

	gettimeofday(&after, NULL);

#if PCMON==1
	SystemCounterState after_sstate = m->getSystemCounterState();

	double l2 = getL2CacheHitRatio(before_sstate, after_sstate);
	double l3 = getL3CacheHitRatio(before_sstate, after_sstate);
	uint64_t reads = getBytesReadFromMC(before_sstate, after_sstate);
	uint64_t writes = getBytesWrittenToMC(before_sstate, after_sstate);
	printf("l2 hit %lf. l3 hit %lf. r %lu. w %lu\n", l2, l3, reads, writes)																																																																	;

#endif


#ifndef NO_PAPI
	if (PAPI_read(EventSet, values) != PAPI_OK){
		fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(retval));
		exit(1);
	}
#endif

	features_t ftafter = get_features(buffer, valueCount, pivot);

	assert(ftafter.sum == ftbefore.sum);
	assert(ftafter.gepivot == ftbefore.gepivot);

	int32_t fraction = (int32_t)((((double)(ftafter.gepivot))/(valueCount) * 100) + 0.5) ;

		printf("{\"experiment\": \"%s\", \"sizemb\": %lld, ", argv[0], memorySize);
#ifndef NO_PAPI
		for (int i = 0; i < sizeof(events)/sizeof(events[0]); i++)
			printf("\"%s\": %lld, ", events[i], values[i]);			
#endif
		printf("\"wallclockmilli\": %ld, "
				"\"partitioned\":%d, "
				"\"ge_pivot\": %lu, "
				"\"lt_fraction\": %d, "
				"\"sum\": \"%016lux\", "
				"\"pivot\": %d, "
				"\"distr\": \"%s\"}\n",
				timediff(before, after)/1000,
				ftafter.partitioned,
				valueCount - ftafter.gepivot,
				fraction,
				ftafter.sum,
				pivotrel,
				distribution);


	free(buffer);
	if(SHUFFLE_PAYLOAD) free(payloadBuffer);

	return 0;
}
