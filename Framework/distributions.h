#ifndef _DISTRIBUTIONS_H_
#define _DISTRIBUTIONS_H_

#include "interface.h"

void randomDistribution(targetType *buffer, unsigned int size, targetType domain, int seed);
void uniformDistribution(targetType *buffer, unsigned int size, targetType domain, int seed);
void skewedDistribution(targetType *buffer, unsigned int size, targetType domain, int seed, int skew);
void holgerDistribution(targetType *buffer, unsigned int size);
void sortedData(targetType *buffer, unsigned int size, targetType domain, int seed);
void revsortedData(targetType *buffer, unsigned int size, targetType domain, int seed);
void almostsortedData(targetType *buffer, unsigned int size, targetType domain, int seed, int step);
int cmpfunc (const void * a, const void * b);

#endif /*_DISTRIBUTIONS_H_*/
