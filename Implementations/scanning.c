#include "../Framework/interface.h"
#include <omp.h>

targetType* performCrack(targetType* buffer, targetType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	targetType* result = (targetType*)malloc(sizeof(targetType)*bufferSize);
	{
//		size_t i = (bufferSize*omp_get_thread_num())/omp_get_num_threads();
//		size_t end = (bufferSize*(omp_get_thread_num()+1))/omp_get_num_threads();
//		size_t outI = (bufferSize*omp_get_thread_num())/omp_get_num_threads();
	#pragma omp parallel for
		for (size_t i = 0; i < bufferSize; i+=64) {
			for (int j = 0; j < 64; j++) {
				result[i] += buffer[i + j] < pivot;
			}
		}
	}

	return result;
}
