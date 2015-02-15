#include "../Framework/interface.h"
#include "tmmintrin.h"

typedef struct
{
	int left, right;
} cursorDeltas;

typedef int v4si __attribute__ ((vector_size (16)));
typedef union {
	int scalar[4];
	v4si vector;
} avxVector;

static inline void avx_memcpy(void* restrict dest, void* restrict src, const unsigned int n){
	for (int i = 0; i < (n/sizeof(v4si)); i++)
		((v4si*)dest)[i] = ((v4si*)src)[i];
}


static inline cursorDeltas performCrackOnVectors(const targetType* restrict scalarInput, targetType* restrict scalarLeftOutput, targetType* restrict scalarRightOutput, const targetType scalarPivot){
	avxVector* input = (avxVector*) scalarInput;
	avxVector* leftOutput = (avxVector*) scalarLeftOutput;
	avxVector* rightOutput = (avxVector*) scalarRightOutput;
	const avxVector pivot = {.scalar = {scalarPivot, scalarPivot, scalarPivot, scalarPivot}};
	const avxVector zeros = {.scalar = {}};
	const avxVector ones = {.scalar = {1,1,1,1}};
	int 
		rightOutI = 0,
		leftOutI = 0,
		inI;
	for (inI = 0; inI < 2*VECTORSIZE/(sizeof(v4si)/sizeof(int)); inI++){
		const v4si vectorMatch = (input[inI].vector >= pivot.vector);
		const avxVector matchesInThisWord = {.vector = (vectorMatch&ones.vector)};
		const avxVector antiMatchesInThisWord = {.vector = ((~vectorMatch)&ones.vector)};
		for (int vectorI = 0; vectorI < sizeof(v4si)/sizeof(int); vectorI++){
			scalarRightOutput[rightOutI] = input[inI].scalar[vectorI];
			scalarLeftOutput[leftOutI] = input[inI].scalar[vectorI];
			rightOutI-=matchesInThisWord.scalar[vectorI];
			leftOutI+=antiMatchesInThisWord.scalar[vectorI];
		}
	}
	
	return (cursorDeltas){.left = leftOutI, .right = rightOutI};
}


targetType* performCrack(targetType* restrict const buffer, targetType* restrict const payloadBuffer, size_t valueCount, targetType pivot,const targetType pivot_P) {
	//TODO: implement the payload shuffling
	assert(valueCount%(2*VECTORSIZE) == 0);
	size_t lowerReadCursor = 0, upperReadCursor = valueCount - VECTORSIZE;
	size_t lowerWriteCursor = 0, upperWriteCursor = valueCount-1;
	targetType buffer1[VECTORSIZE*2], buffer2[VECTORSIZE*2];

	avx_memcpy(buffer1, buffer+lowerReadCursor, sizeof(targetType)*VECTORSIZE);
	avx_memcpy(buffer1+VECTORSIZE, buffer+upperReadCursor, sizeof(targetType)*VECTORSIZE);
	lowerReadCursor += VECTORSIZE;
	upperReadCursor -= VECTORSIZE;
	
	while (lowerWriteCursor < upperWriteCursor) {
		{
			avx_memcpy(buffer2, buffer+lowerReadCursor, sizeof(targetType)*VECTORSIZE);
			avx_memcpy(buffer2+VECTORSIZE, buffer+upperReadCursor, sizeof(targetType)*VECTORSIZE);
			lowerReadCursor += VECTORSIZE;
			upperReadCursor -= VECTORSIZE;
			cursorDeltas deltas = performCrackOnVectors(buffer1, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot);
			lowerWriteCursor += deltas.left;
			upperWriteCursor += deltas.right;
		}
		{
			avx_memcpy(buffer1, buffer+lowerReadCursor, sizeof(targetType)*VECTORSIZE);
			avx_memcpy(buffer1+VECTORSIZE, buffer+upperReadCursor, sizeof(targetType)*VECTORSIZE);
			lowerReadCursor += VECTORSIZE;
			upperReadCursor -= VECTORSIZE;
			cursorDeltas deltas = performCrackOnVectors(buffer2, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot);		
			lowerWriteCursor += deltas.left;
			upperWriteCursor += deltas.right;
			
		}
	}
	return NULL;
}

/* Local Variables: */
/* compile-command: "make -k -C .. vectorized" */
/* End: */
