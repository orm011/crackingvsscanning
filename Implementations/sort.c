#include "../Framework/interface.h"
#include <parallel/algorithm>

targetType* performCrack(targetType* buffer, payloadType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	assert(payloadBuffer == NULL);
  __gnu_parallel::sort(buffer, buffer+bufferSize+1);
}
