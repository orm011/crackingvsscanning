#ifndef _CRACKING_MT_NOTMERGE_H_
#define _CRACKING_MT_NOTMERGE_H_

#include "interface.h"
#include "threadpool.h"

typedef void (*IdleFuncPtr)(c_Thread_t *);
IdleFuncPtr IdleFunction;
void standard_cracking_revised_notmerge ( targetType *attribute,  payloadType* payloadBuffer, const targetType pivot, const size_t first, const size_t last, const size_t ml, const size_t mr, const targetType pivot_P);
void standard_cracking_revised_notmerge_x ( targetType *attribute, payloadType* payloadBuffer,  const targetType pivot, const size_t first, const size_t last, const size_t ml, const size_t mr, const targetType pivot_P);
void cracking_MT_crackThread_notmerge ( c_Thread_t *arg_p );
void cracking_MT_notmerge (size_t first, size_t last, targetType *b, payloadType* payloadBuffer,  targetType pivot, int nthreads, int alt, const targetType pivot_P); /*alternative 1+2*/

#endif /*_CRACKING_MT_H_*/
