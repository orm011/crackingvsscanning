#include "../Framework/interface.h"

#include "common.h"


targetType* performCrack(targetType* restrict const buffer, targetType* restrict const payloadBuffer, size_t valueCount, targetType pivot,const targetType pivot_P) {
//TODO: implement the payload reshuffling
	assert(valueCount%(2*ELEMENTS_PER_VECTOR) == 0);
	const size_t vectorCount = valueCount/ELEMENTS_PER_VECTOR;
	size_t lowerReadCursor = 0, upperReadCursor = valueCount;   //instead of valueCount-1
	size_t lowerWriteCursor = 0, upperWriteCursor = valueCount-1;
	targetType localBuffer[ELEMENTS_PER_VECTOR*3];

	__builtin_memcpy(localBuffer, buffer, sizeof(targetType)*2*ELEMENTS_PER_VECTOR);
	__builtin_memcpy(localBuffer+2*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);
	lowerReadCursor += 2*ELEMENTS_PER_VECTOR;
	upperReadCursor -= ELEMENTS_PER_VECTOR;

	for (size_t vectorI = 0; vectorI < vectorCount; vectorI++)	{
		cursorDeltas deltas = performCrackOnVectors(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot);
		lowerWriteCursor += deltas.left;
		upperWriteCursor += deltas.right;
		if(lowerReadCursor-lowerWriteCursor < upperWriteCursor-upperReadCursor){
			__builtin_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+lowerReadCursor, sizeof(targetType)*ELEMENTS_PER_VECTOR);
			lowerReadCursor+=ELEMENTS_PER_VECTOR;
		} else {
			__builtin_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);
			upperReadCursor-=ELEMENTS_PER_VECTOR;
		}
	}
	return NULL;
}

/* Local Variables: */
/* compile-command: "make -k -C .. vectorized" */
/* End: */
