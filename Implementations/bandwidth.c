#include "../Framework/interface.h"
#include <omp.h>

targetType* performCrack(targetType* buffer, targetType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	targetType* result = (targetType*)malloc(sizeof(targetType)*bufferSize);
#pragma omp parallel
	{
		size_t i = (bufferSize*omp_get_thread_num())/omp_get_num_threads();
		size_t end = (bufferSize*(omp_get_thread_num()+1))/omp_get_num_threads();
		size_t outI = (bufferSize*omp_get_thread_num())/omp_get_num_threads();
		for (; i < end; i+=64){
			for (int j = 0; j < 64; j++){
				result[outI] = buffer[i+j];
				outI += (buffer[i+j] < pivot);
			}
		}
	} 

	return result;
}
