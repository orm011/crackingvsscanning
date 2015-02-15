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
	
	which1 = 0;
	word1 = ((((long)buffer[higher])<<32)+buffer[lower]);
	which2 = 1;
	word2 = ((((long)buffer[higher])<<32)+buffer[lower]);

	for (i = 0; i < valueCount; i+=2){
		{		
			const targetType value1 =	(word1>>(32*which1))&BitMask;
			buffer[lower] = buffer[higher] = value1;
			const int advanceLower = (value1<pivot);
			const int advanceHigher = (value1>=pivot);
			lower+=advanceLower;
			higher-=advanceHigher;
			word1 = ((((long)buffer[higher])<<32)+buffer[lower]);
			which1 = advanceHigher;
		}
		{		
			const targetType value2 =	(word2>>(32*which2))&BitMask;
			buffer[lower] = buffer[higher] = value2;
			const int advanceLower = (value2<pivot);
			const int advanceHigher = (value2>=pivot);
			lower+=advanceLower;
			higher-=advanceHigher;
			word2 = ((((long)buffer[higher])<<32)+buffer[lower]);
			which2 = advanceHigher;
		}
	}

}	
