#include "../Framework/cracking_MT_vectorized.h"
targetType* performCrack(targetType* buffer, payloadType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	//assert(buffer);
	size_t pos;
	//int nthreads=4;

	cracking_MT_vectorized(0, bufferSize-1, buffer, payloadBuffer, pivot, &pos, NTHREADS, 1, pivot_P);
	return NULL;
}
