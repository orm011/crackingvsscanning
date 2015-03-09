#include "../Framework/interface.h"

targetType* performCrack(targetType* buffer, targetType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	targetType* result = (targetType*)malloc(sizeof(targetType)*bufferSize);

	_Cilk_for (int i = 0; i < bufferSize; ++i) {
		result[i] = buffer[i];
	}

	return result;
}
