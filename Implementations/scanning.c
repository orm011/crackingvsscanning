#include "../Framework/interface.h"
#include <numeric>


size_t scanBase(targetType* buffer, size_t size, targetType pivot) {
	size_t total = 0;

	if (size < 8192*2) {
		for (size_t i = 0; i < size; ++i) {
			total += buffer[i] >= pivot;
		}
		return total;
	} else {
		size_t half = size/2;
		size_t ta = _Cilk_spawn scanBase(buffer, half, pivot);
		size_t tb = scanBase(buffer + half, half, pivot);
		_Cilk_sync;
		total = ta + tb;
	}

	return total;
}

targetType* performCrack(targetType* buffer, targetType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	targetType* result = (targetType*)malloc(sizeof(targetType)*bufferSize);

	size_t total = scanBase(buffer, bufferSize, pivot);
	printf("total %lu", total);
	result[0] = total;
	return result;
}
