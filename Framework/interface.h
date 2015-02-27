#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

typedef unsigned int targetType;
typedef targetType payloadType;

#define int_nil INT_MIN
#define BUN_NONE ((size_t) INT_MAX)

static const char* randomD = "randomD";
static const char* uniformD = "uniformD";
static const char* skewedD = "skewedD";
static const char* holgerD = "holgerD";
static const char* sortedD = "sortedD";
static const char* revsortedD = "revsortedD";
static const char* almostsortedD = "almostsortedD";

/* argument struct for countThread & crackThread functions */
typedef struct {
        targetType *b;      /* BAT to be cracked */
        payloadType *payload;      /* BAT to be cracked */
        targetType *pivot;  /* pivot value */
        int pivotR; /* pivot relative percent */
        size_t first;         /* offset of first value in slice */
        size_t last;          /* offset of last value in slice */
        size_t pos_r;           /* offset of pivot value */
        size_t ml;            /* size of left half slice */
        size_t mr;            /* size of right half slice */
} c_Thread_t;


targetType* performCrack(targetType* buffer, payloadType* payloadBuffer, size_t bufferSize, targetType pivot,  const targetType pivot_P);
void create_values(const char *distribution, targetType* buffer, unsigned int size, targetType domain);

#endif /* _INTERFACE_H_ */
