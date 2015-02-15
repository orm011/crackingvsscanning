#include "../Framework/interface.h"

typedef struct
{
	char which;
	targetType values[2];
} slot;

targetType* performCrack(targetType* restrict const buffer, targetType* restrict const payloadBuffer, size_t valueCount, targetType pivot,const targetType pivot_P) {
	size_t i, lower=0, higher=valueCount-1;
	slot localbuffer[2] = {
		{.which = 0, .values = {buffer[lower], buffer[higher]}},
		{.which = 1, .values = {buffer[lower], buffer[higher]}}
	};
	for (i = 0; i < valueCount; i++){
		const targetType value = localbuffer[0].values[i%2];
    buffer[lower] = buffer[higher] = value;
		const int advanceLower = (value<pivot);
		const int advanceHigher = (value>=pivot);
		localbuffer[0].values[i%2] = advanceHigher*buffer[higher-1]+(advanceLower)*buffer[lower+1];
		lower+=advanceLower;
		higher-=advanceHigher;
		/* localbuffer[0] = (slot){ */
		/* 	/\* .which = advanceHigher, *\/ */
		/* 	.values = {buffer[lower], buffer[higher]} */
		/* }; */
	}

}	
