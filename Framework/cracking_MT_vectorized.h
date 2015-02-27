#ifndef _CRACKING_MT_VECTORIZED_H_
#define _CRACKING_MT_VECTORIZED_H_

#include "interface.h"
#include "threadpool.h"

typedef void (*IdleFuncPtr)(c_Thread_t *);
IdleFuncPtr IdleFunction;
void cracking_vectorized (targetType* __restrict__ const buffer, payloadType* payloadBuffer, const targetType pivot, const size_t first, const size_t last, const size_t ml, const size_t mr, size_t *pos_r, const targetType pivot_P);
void cracking_vectorized_x (targetType* __restrict__ const buffer, payloadType* payloadBuffer, const targetType pivot, const size_t first, const size_t last, const size_t ml, const size_t mr, size_t *pos_r, const targetType pivot_P);
void cracking_MT_vectorized_crackThread ( c_Thread_t *arg_p );
void cracking_MT_vectorized (size_t first, size_t last, targetType *b, payloadType* payloadBuffer, targetType pivot, size_t *pos, int nthreads, int alt, const targetType pivot_P); /*alternative 1+2*/

#endif /*_CRACKING_MT_H_*/
