#include "../Framework/cracking_MT_notmerge.h"
targetType* performCrack(targetType* buffer, payloadType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	cracking_MT_notmerge(0, bufferSize-1, buffer, payloadBuffer, pivot, NTHREADS, 1, pivot_P);
	return NULL;
}
