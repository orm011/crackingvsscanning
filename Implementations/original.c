#include "../Framework/cracking_MT.h"
targetType* performCrack(targetType* buffer, payloadType* payloadBuffer,size_t bufferSize, targetType pivot, const targetType pivot_P) {
	//assert(buffer);
	size_t pos;

	standard_cracking_revised(buffer, payloadBuffer, pivot, 0, bufferSize-1, 0, 0, &pos, pivot_P);
	return NULL;
}
