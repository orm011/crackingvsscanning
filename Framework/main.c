#include <stdio.h>
#include "interface.h"
#ifndef NO_PAPI
#include "papi.h"

#endif
#include <sys/time.h>
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

int main(int argc, char* argv[]) {
	size_t memorySize = (argc > 1 ?atoi(argv[1]):2048);
	const size_t valueCount = memorySize * 1024 * 1024 / sizeof(targetType);
	targetType* buffer;
	posix_memalign((void**)(&buffer), 32, valueCount * sizeof(targetType));
	payloadType* payloadBuffer = (SHUFFLE_PAYLOAD)?(payloadType*) malloc(valueCount * sizeof(payloadType)):NULL;
	unsigned long long sum_before=0, sum_after=0, sum_prod_val_pos_before=0, sum_prod_val_pos_after=0;

	assert(PIVOT>=0 && PIVOT<=100);
	targetType pivot = (valueCount * PIVOT) / (100.0);

	create_values(DISTRIBUTION, buffer, valueCount, valueCount);
		
	for (size_t i = 0; i < valueCount; i++){
		if(SHUFFLE_PAYLOAD)
		{
			payloadBuffer[i] = i;
			sum_prod_val_pos_before += (buffer[i] * payloadBuffer[i]);
		}
		sum_before += buffer[i];   //used for result validation
	}

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
	gettimeofday(&before, NULL);

	performCrack(buffer, payloadBuffer, valueCount, pivot, PIVOT);

	gettimeofday(&after, NULL);

#ifndef NO_PAPI
	if (PAPI_read(EventSet, values) != PAPI_OK){
		fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(retval));
		exit(1);
	}
#endif

	{
		size_t lastSmaller = 0, firstGreater = 0, everyFirstValueIsZero = 1;
		for (size_t i = 0; i < valueCount; i++) {
			if(buffer[i] < pivot)
				lastSmaller = i;
			else if(firstGreater == 0){
				firstGreater = i;
			}
			if((i%16 == 0) && buffer[i] != 0){
				everyFirstValueIsZero = 0;
			}
			sum_after += buffer[i];	
			if(SHUFFLE_PAYLOAD)
				sum_prod_val_pos_after += (buffer[i] * payloadBuffer[i]);

		}
		if (lastSmaller == valueCount - 1 && firstGreater == 0) {
			/* PIVOT == 100% */
			firstGreater = valueCount;
		}

		printf("{\"experiment\": \"%-60s\", \"fieldsize\": %lu, ", argv[0], memorySize);
#ifndef NO_PAPI
		for (int i = 0; i < sizeof(events)/sizeof(events[0]); i++)
			printf("\"%s\": %lld, ", events[i], values[i]);			
#endif
		printf("\"WALLCLOCK\": %9ld, \"PROPER\": %d, \"PROPERZEROEDOUT\": %zu, \"SUMCOMP:\": %d, \"SUM_PROD_COMP:\": %d, \"PIVOT\": %d}\n", timediff(before, after), (lastSmaller<firstGreater), everyFirstValueIsZero,(sum_after==sum_before),(sum_prod_val_pos_before == sum_prod_val_pos_after), PIVOT);
	}


	free(buffer);
	if(SHUFFLE_PAYLOAD) free(payloadBuffer);

	return 0;
}
