#include "../Framework/interface.h"
#include <alloca.h>
//#include <parallel/algorithm>

#ifndef _OPENMP
static int omp_get_thread_num(){
	return 0;
}
static int omp_get_num_threads(){
	return 1;
}
static int omp_get_max_threads(){
	return 1;
}
#else
#include <omp.h>
#endif



void
randomDistribution(targetType *buffer, unsigned int size, targetType domain, int seed)
{
	unsigned int p = 0, q = size;
	unsigned int* rbuf = NULL;
	size_t linesize = 64;
	int r= posix_memalign((void**)&rbuf, linesize, omp_get_max_threads()*linesize);
	assert(r == 0);

	for (int i = 0; i < omp_get_max_threads(); i+=linesize) {
		rbuf[i] = ((unsigned)seed) + i;
	}

	assert(size > 0);

	#pragma omp parallel for
	for (int i=0; i < q; i++)
		buffer[i] = rand_r(rbuf+omp_get_thread_num()*linesize);
}

void
uniformDistribution(targetType *buffer, unsigned int size, targetType domain, int seed)
{
	unsigned int i, r=0, firstbun=0, p = 0, q = size;
	unsigned int j = 0;
	unsigned int* rbuf = (unsigned int*) alloca(omp_get_max_threads()*sizeof(unsigned int));

	assert(size>0);

	if (seed != int_nil)
		srand(seed);

	/* create values with uniform distribution */
	#pragma omp parallel for
	for (int i = 0; i < q; i++){
		buffer[i] = j;
		if (++j >= domain)
			j = 0;
	}

	/* mix values randomly */
	//#pragma omp parallel for
	for (int i = 0; i < size; i++) {
		unsigned int idx = i + ((r += (unsigned int) rand_r(rbuf+omp_get_thread_num())) % (size - i));
		targetType val;

		p = firstbun + i;
		q = firstbun + idx;
		val = buffer[p];
		buffer[p] = buffer[q];
		buffer[q] = val;
	}
}

void
skewedDistribution(targetType *buffer, unsigned int size, targetType domain, int seed, int skew)
{
	unsigned int i, r=0, firstbun=0, lastbun, p = 0, q = size;
	unsigned int skewedSize;
	unsigned int* rbuf = (unsigned int*) alloca(omp_get_max_threads()*sizeof(unsigned int));
	int skewedDomain;

	assert(size>0);

	if (seed != int_nil)
		srand(seed);

	/* create values with skewed distribution */
	skewedSize = ((skew) * size)/100;
	skewedDomain = ((100-(skew)) * (domain))/100;

	lastbun = firstbun + skewedSize;
	#pragma omp parallel for
	for(i=firstbun; i <lastbun; i++)
	{
		buffer[i] = (int)rand_r(rbuf+omp_get_thread_num()) % skewedDomain;
		p++;
	}

	lastbun = size;
	#pragma omp parallel for
	for(i=p; i <lastbun; i++)
		buffer[i] = ((int)rand_r(rbuf+omp_get_thread_num()) % (domain-skewedDomain)) + skewedDomain;

	/* mix values randomly */
	//#pragma omp parallel for
	for (i = 0; i < size; i++) {
		unsigned int idx = i + ((r += (unsigned int) rand_r(rbuf+omp_get_thread_num())) % (size - i));
		int val;

		p = firstbun + i;
		q = firstbun + idx;
		val = buffer[p];
		buffer[p] = buffer[q];
		buffer[q] = val;
	}
}

void holgerDistribution(targetType *buffer, unsigned int size)
{
	for (size_t i = 0; i < size; i++)
		buffer[i] = ((i * 179123) << (i % 32)) % size;
}

int cmpfunc (const void * a, const void * b)
{
   return ( *(targetType*)a - *(targetType*)b );
}

void sortedData(targetType *buffer, unsigned int size, targetType domain, int seed)
{
	randomDistribution(buffer, size, domain, seed);
	//__gnu_parallel::sort(buffer, buffer+size+1);
	
	qsort (buffer, size, sizeof (targetType), cmpfunc);
}

void revsortedData(targetType *buffer, unsigned int size, targetType domain, int seed)
{
	size_t i=0, j=size-1;
	targetType temp;

	randomDistribution(buffer, size, domain, seed);

	qsort (buffer, size, sizeof (targetType), cmpfunc);

	while(i<j)
	{
		temp=buffer[i];
		buffer[i]=buffer[j];
		buffer[j]=temp;
		i++;
		j--;
	}
}

void almostsortedData(targetType *buffer, unsigned int size, targetType domain, int seed, int step)
{
	int i=0,n=0,j=0;
	targetType val=0,temp;

	if (seed != int_nil)
		srand(seed);

	randomDistribution(buffer, size, domain, seed);

	qsort (buffer, size, sizeof (targetType), cmpfunc);

	for (i = 0; i < size; i++)
	{
		n=rand()%100;
		if(n==1 && (i+step)<size)
		{
			j=rand() % step + i;
			temp=buffer[i];
			buffer[i] = buffer[j];
			buffer[j]=buffer[i];
		}
	}
}
