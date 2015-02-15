#include "../Framework/interface.h"

typedef struct
{
	int left, right;
} cursorDeltas;

typedef int v4si __attribute__ ((vector_size (16)));

static inline void avx_memcpy(void* restrict dest, void* restrict src, const unsigned int n){
	for (int i = 0; i < (n/sizeof(v4si)); i++)
		((v4si*)dest)[i] = ((v4si*)src)[i];
}


static inline cursorDeltas performCrackOnVectors(const targetType* restrict input/* , targetType* rightInput */, targetType* restrict leftOutput, targetType* restrict rightOutput, const targetType pivot){
	int 
		rightOutI = 0,
		leftOutI = 0,
		inI;
	for (inI = 0; inI < 2*VECTORSIZE; inI++){
		rightOutput[rightOutI] = input[inI];
		const unsigned int  isLessThan = (input[inI] < pivot);
		rightOutI -= (~isLessThan)&1;
		leftOutput[leftOutI] = input[inI];
		leftOutI += isLessThan;
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
