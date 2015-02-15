#include "../Framework/interface.h"

typedef struct
{
	long asWord;
} slot;

const unsigned int BitMask = UINT_MAX;
targetType* performCrack(targetType* restrict const buffer, targetType* restrict const payloadBuffer, size_t valueCount, targetType pivot,const targetType pivot_P) {
	size_t i, lower=0, higher=valueCount-1;
	char which1, which2;
	long word1, word2;
	
	word1 = ((((long)buffer[higher])<<32)+buffer[lower]);

	for (i = 0; i < valueCount; i++){
		const targetType value1 =	(word1>>(32*(i%2)))&BitMask;
		buffer[lower] = buffer[higher] = value1;
		const int advanceLower = (value1<pivot);
		const int advanceHigher = (value1>=pivot);
		const long nextValue = advanceHigher*buffer[higher-1]+(advanceLower)*buffer[lower+1];
		lower+=advanceLower;
		higher-=advanceHigher;
		const long otherValue1 =	(word1)&(((unsigned long)BitMask)<<(32*(1-i%2)));
		word1 = (otherValue1) + (nextValue<<(32*(i%2)));
	}
	
}	
