#include "../Framework/cracking_MT_vectorized.h"
#include <sys/time.h>
//#include "common.h"
#include <stdint.h>
#if PCMON == 1
#include <cpucounters.h>
#endif

static long timediff(struct timeval before, struct timeval after){
	return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}


typedef struct {
	ssize_t left, right;
} cursorDeltas;

const size_t ELEMENTS_PER_VECTOR = VECTORSIZE/sizeof(targetType);


static inline void * naive_memcpy(void * dst, const void * src, size_t num) {
	typedef uint64_t t;

	const t *srcfoo = (const t *)(src);
	t * dstfoo = (t *)(dst);
	const t *end = (t *)(((char*)dst) + num);


	for (; dstfoo < end; dstfoo+=1, srcfoo+=1) {
		*dstfoo  = *srcfoo;
	}

	return dst;
}


static inline cursorDeltas performCrackOnVectors(const targetType* __restrict__ input, targetType* __restrict__ leftOutput, targetType* __restrict__ rightOutput, const targetType pivot
){
	ssize_t
	rightOutI = 0,
	leftOutI = 0,
	inI;
	for (inI = 0; inI < ELEMENTS_PER_VECTOR; inI++){
		rightOutput[rightOutI] = input[inI]; // this step is always done unconditionally. at which point do we restore the original.
		const unsigned int  isLessThan = (input[inI] < pivot);
		rightOutI -= (~isLessThan)&1;
		leftOutput[leftOutI] = input[inI];
		leftOutI += isLessThan;
	}
	return (cursorDeltas){leftOutI, rightOutI};
}

static inline cursorDeltas performCrackOnVectors_left(const targetType* __restrict__ input, targetType* __restrict__ leftOutput, targetType* __restrict__ rightOutput, const targetType pivot
		, ssize_t max, size_t skip
){
	ssize_t
	rightOutI = 0,
	leftOutI = 0,
	inI;
	for (inI = 0; inI < ELEMENTS_PER_VECTOR; inI++){
		rightOutput[rightOutI] = input[inI];
		const unsigned int  isLessThan = (input[inI] < pivot);
		rightOutI -= (~isLessThan)&1;
		leftOutput[leftOutI] = input[inI];
		leftOutI += isLessThan;
		leftOutI += (leftOutI == max) * skip;
	}
	return (cursorDeltas){leftOutI, rightOutI};
}

static inline cursorDeltas performCrackOnVectors_right(const targetType* __restrict__ input, targetType* __restrict__ leftOutput, targetType* __restrict__ rightOutput, const targetType pivot
		, ssize_t min, size_t skip
){
	ssize_t
	rightOutI = 0,
	leftOutI = 0,
	inI;
	for (inI = 0; inI < ELEMENTS_PER_VECTOR; inI++){
		rightOutput[rightOutI] = input[inI];
		const unsigned int  isLessThan = (input[inI] < pivot);
		rightOutI -= (~isLessThan)&1;
		rightOutI -= (rightOutI == min) * skip;
		leftOutput[leftOutI] = input[inI];
		leftOutI += isLessThan;
	}
	return (cursorDeltas){leftOutI, rightOutI};
}

static inline cursorDeltas performCrackOnVectors_left_right(const targetType* __restrict__ input, targetType* __restrict__ leftOutput, targetType* __restrict__ rightOutput, targetType pivot
		, ssize_t max, ssize_t min, size_t skip
){
	ssize_t
	rightOutI = 0,
	leftOutI = 0,
	inI;
	for (inI = 0; inI < ELEMENTS_PER_VECTOR; inI++){
		rightOutput[rightOutI] = input[inI];
		const unsigned int  isLessThan = (input[inI] < pivot);
		rightOutI -= (~isLessThan)&1;
		rightOutI -= (rightOutI == min) * skip;
		leftOutput[leftOutI] = input[inI];
		leftOutI += isLessThan;
		leftOutI += (leftOutI == max) * skip;
	}
	return (cursorDeltas){leftOutI, rightOutI};
}

/* revised single-threaded crack code (used in multicore alternatives)*/
void
cracking_vectorized (
		/* input */
		targetType* __restrict__ const attribute,      /* attribute (array) */
		payloadType* payloadBuffer,
		targetType pivot, /* pivot value for attribute*/
		size_t first,    /* first position of to-be-cracked piece */
		size_t last,    /* last position of to-be-cracked piece */
		size_t ml,
		size_t mr,
		size_t *pos, /*return position*/
		const targetType pivot_P
) {
	size_t pos_r;

	//assert(attribute);
	//assert(pos);
	//assert((ml == 0 && mr == 0) || first + ml - 1 + 1 == last + 1 - mr);

	cracking_vectorized_x ( attribute, payloadBuffer, pivot, first, last, ml, mr, &pos_r, pivot_P);
	*pos = (size_t) (pos_r == 0 ? 0 : pos_r - 1);
}
void
cracking_vectorized_x (
		/* input */
		targetType* __restrict__ const buffer,      /* attribute (array) */
		payloadType* payloadBuffer,
		const targetType pivot, /* pivot value for attribute*/
		size_t first_left,  /* first position of to-be-cracked piece */
		size_t last_right,  /* last position of to-be-cracked piece */
		size_t ml,
		size_t mr,
		size_t *pos_r, /*return position*/
		const targetType pivot_P
) {
	struct timeval tac;
	struct timeval tbc;
	struct timeval forstart = {0,0};
	struct timeval forend = {0,0};

	gettimeofday(&tac, NULL);
	const size_t ls = first_left;
	const size_t le = first_left + ml;
	const size_t rs = last_right +1 - mr;
	const size_t re = last_right+1;

	const size_t mlc = ml;
	const size_t mrc = mr;

	size_t last_left = first_left + ml - 1, first_right = last_right + 1 - mr;
	assert(!(ml && mr && last_left + 1 < first_right) || (ml%(2*ELEMENTS_PER_VECTOR) == 0 && mr%(2*ELEMENTS_PER_VECTOR) == 0));
	size_t valueCount = ( (ml && mr && last_left + 1 < first_right) ?  ml + mr : last_right - first_left + 1 );
	assert(valueCount%(2*ELEMENTS_PER_VECTOR) == 0);
	const size_t vectorCount = valueCount/ELEMENTS_PER_VECTOR;
	size_t lowerReadCursor = first_left, upperReadCursor = last_right + 1;
	size_t lowerWriteCursor = first_left, upperWriteCursor = last_right;
	targetType localBuffer[ELEMENTS_PER_VECTOR*3]; // local buffer? what for?
	//assert(attribute);
	//assert(pos_r);

	// copy from two potentially separate places into one location (predication?)
	naive_memcpy(localBuffer, buffer+first_left, sizeof(targetType)*2*ELEMENTS_PER_VECTOR);
	naive_memcpy(localBuffer+2*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);

	lowerReadCursor += 2*ELEMENTS_PER_VECTOR;
	upperReadCursor -= ELEMENTS_PER_VECTOR;


	size_t vectorI = 0, vectorR = 3;

	if (ml && mr && last_left + 1 < first_right) {
		gettimeofday(&forstart, NULL);
		/* we have two disjoint half-pieces */
		for (; vectorI < vectorCount; vectorI++)	{
			assert (lowerWriteCursor <= upperWriteCursor);
			if (lowerWriteCursor >= first_right) {
				/* do remainder on consecutive right half */
				ml = mr = 0;
				first_left = first_right;
				last_left = last_right;
				break;
			}
			if (upperWriteCursor <= last_left) {
				/* do remainder on consecutive left half */
				ml = mr = 0;
				first_right = first_left;
				last_right = last_left;
				break;
			}
			assert (vectorR >= vectorCount || lowerWriteCursor + ELEMENTS_PER_VECTOR <= lowerReadCursor);
			assert (vectorR >= vectorCount || upperWriteCursor - ELEMENTS_PER_VECTOR >= upperReadCursor - 1);
			size_t skip = first_right - last_left - 1;
			cursorDeltas deltas;
			if (lowerWriteCursor <= last_left && lowerWriteCursor > last_left - ELEMENTS_PER_VECTOR + 1 &&
					upperWriteCursor >= first_right && upperWriteCursor < first_right + ELEMENTS_PER_VECTOR - 1) {
				deltas = performCrackOnVectors_left_right(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot
						, (ssize_t) last_left - lowerWriteCursor + 1, (ssize_t) first_right - upperWriteCursor - 1, skip
				);
			} else
				if (lowerWriteCursor <= last_left && lowerWriteCursor > last_left - ELEMENTS_PER_VECTOR + 1) {
					deltas = performCrackOnVectors_left(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot
						, (ssize_t) last_left - lowerWriteCursor + 1, skip
					);
				} else
					if (upperWriteCursor >= first_right && upperWriteCursor < first_right + ELEMENTS_PER_VECTOR - 1) {
						deltas = performCrackOnVectors_right(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot
								, (ssize_t) first_right - upperWriteCursor - 1, skip
						);
					} else {
						deltas = performCrackOnVectors(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot
						);
					}
			lowerWriteCursor += deltas.left;
			upperWriteCursor += deltas.right;

			// are these ever taken, if the skip stuff in the performCrackOnVectors is longer?
			if (lowerWriteCursor == last_left + 1)
				lowerWriteCursor = first_right;
			if (upperWriteCursor == first_right - 1)
				upperWriteCursor = last_left;
			if (vectorR < vectorCount) {
				size_t skip_left = 0, skip_right = 0;
				if (lowerWriteCursor < first_right && lowerReadCursor > last_left) {
					skip_left = first_right - last_left;
				}
				if (upperReadCursor - 1 < first_right && upperWriteCursor > last_left) {
					skip_right = first_right - last_left;
				}
				if(lowerReadCursor-lowerWriteCursor-skip_left < upperWriteCursor-(upperReadCursor-1)-skip_right){
					naive_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+lowerReadCursor, sizeof(targetType)*ELEMENTS_PER_VECTOR);
					lowerReadCursor+=ELEMENTS_PER_VECTOR;
					if (lowerReadCursor == last_left + 1)
						lowerReadCursor = first_right;
				} else {
					naive_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);
					upperReadCursor-=ELEMENTS_PER_VECTOR;
					if (upperReadCursor == first_right)
						upperReadCursor = last_left + 1;
				}
				vectorR++;
			}
		}
		gettimeofday(&forend, NULL);

	} else {
		ml = mr = 0;
		first_right = first_left;
		last_left = last_right;
	}

	struct timeval forstart2;
	struct timeval forend2;

	gettimeofday(&forstart2, NULL);
	for (; vectorI < vectorCount; vectorI++)	{
		/* we only have one consecutive (half-) piece (remaining) */
		assert (lowerWriteCursor <= upperWriteCursor);
		assert (vectorR >= vectorCount || lowerWriteCursor + ELEMENTS_PER_VECTOR <= lowerReadCursor);
		assert (vectorR >= vectorCount || upperWriteCursor - ELEMENTS_PER_VECTOR >= upperReadCursor - 1);
		cursorDeltas deltas = performCrackOnVectors(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot
		);
		lowerWriteCursor += deltas.left;
		upperWriteCursor += deltas.right;
		if (vectorR < vectorCount) {
			if(lowerReadCursor-lowerWriteCursor < upperWriteCursor-(upperReadCursor-1)){
				naive_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+lowerReadCursor, sizeof(targetType)*ELEMENTS_PER_VECTOR);
				lowerReadCursor+=ELEMENTS_PER_VECTOR;
			} else {
				naive_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);
				upperReadCursor-=ELEMENTS_PER_VECTOR;
			}
			vectorR++;
		}
	}
	gettimeofday(&forend2, NULL);

	assert (vectorR == vectorCount);
	//assert(lowerReadCursor == upperReadCursor || (lowerReadCursor == first_right && upperReadCursor == last_left + 1));

#ifndef NDEBUG
	ssize_t lwc = (ssize_t) lowerWriteCursor;
	ssize_t uwc = (ssize_t) upperWriteCursor;
	ssize_t fl = (ssize_t) first_left;
	ssize_t ll = (ssize_t) last_left;
	ssize_t fr = (ssize_t) first_right;
	ssize_t lr = (ssize_t) last_right;

	assert(lwc >= fl);
	assert(lwc <= lr + 1);
	assert(uwc >= fl - 1);
	assert(uwc <= lr);
	assert(lwc > uwc);
	assert(uwc == fl - 1 || buffer[uwc] <  pivot);
	assert(lwc == lr + 1 || buffer[lwc] >= pivot);
	if (ml && mr && ll + 1 < fr) {
		if (lwc <= ll || uwc >= fr) {
			assert(lwc == uwc + 1);
		} else {
			assert(lwc == fr && uwc == ll);
		}
	} else {
		assert(lwc == uwc + 1);
	}
#endif
	*pos_r = lowerWriteCursor;
#if TIMING == 1
	gettimeofday(&tbc, NULL);
	long int diff = timediff(tac, tbc);
	long int diffFor1 = timediff(forstart, forend);
	long int diffFor2 = timediff(forstart2, forend2);
	fprintf(stderr, "total: %07ld, firstfor: %07ld, secfor: %07ld, size: %llu, rend %p \n", diff, diffFor1, diffFor2, (le - ls + re - rs), re);
#endif
}
/* crackThread for multi-threaded crack code */
void
cracking_MT_vectorized_crackThread ( c_Thread_t *arg )
{
	size_t pos_r;
	targetType pivot = * (targetType*) arg->pivot;
	targetType pivot_P = arg->pivotR;

	/*call revised cracking routine for this slice */
	cracking_vectorized_x (arg->b, arg->payload, pivot, arg->first, arg->last, arg->ml, arg->mr, &pos_r, pivot_P);

	arg->pos_r = pos_r;
}
/* new multi-threaded crack code; Alternative 1+2 */
void
cracking_MT_vectorized (size_t first, size_t last, targetType *b, payloadType* payloadBuffer, targetType pivot,
		size_t *pos, int nthreads, int alt, const targetType pivot_P)
{


	struct timeval tva;
	struct timeval tvb;
	struct timeval tvc;
	struct timeval tvd;

	gettimeofday(&tva, NULL);

	size_t n = last - first + 1; /* total # tuples / values */
	size_t ml;                   /* # tuples / values in left slice */
	size_t mr;                   /* # tuples / values in right slice */
	size_t mm;                   /* # tuples / values in tmp arrays */
	size_t f, l;                 /* first / last size_t per slice */
	size_t last_vector_pos = last;       /* position of the last element in the last vector*/
	c_Thread_t *c_Thread_arg; /* thread arguments array */
	int i, j;
	targetType *temp;
	size_t remaining_elements = 0; /*elements that do not "fit" in vectors*/

	int oldnhreads = nthreads; //the real number of threads.
	nthreads = nthreads * TASKS_PER_THREAD; //hack to decouple # of tasks from # of threads
	mm = (n / nthreads);

	remaining_elements = (mm % (2 * ELEMENTS_PER_VECTOR)) * nthreads;
	mm = (n - remaining_elements) / nthreads;
	remaining_elements += ((n - remaining_elements) % nthreads);
	last_vector_pos -= remaining_elements;
	assert(((n - remaining_elements) % nthreads) == 0);
	mm -= ((n - remaining_elements) % nthreads);

	if (alt == 1) {
		ml = 0;
		mr = 0;
	} else /* alt == 2 */ {
		ml = (size_t) round(((double) (mm * pivot_P) / 100.0) / (2*ELEMENTS_PER_VECTOR)) * 2*ELEMENTS_PER_VECTOR;
		mr = mm - ml;
		if (ml == 0 || mr == 0) {
			alt = 1;
			ml = 0;
			mr = 0;
		}
	}

#ifdef DO_PAYLOAD_SHUFFLE
	payloadType *tempPayload;
	tempPayload = malloc(mm * sizeof(payloadType));
	if (!tempPayload) {
		if (tempPayload)
			free(tempPayload);
		fprintf (stderr, "cracking_MT(): malloc() failed.\n");
		exit(EXIT_FAILURE);
	}
#endif
	temp = (targetType*)malloc(mm * sizeof(targetType));
	c_Thread_arg  = (c_Thread_t*)malloc(alt * nthreads * sizeof(c_Thread_t));
	if (!temp || !c_Thread_arg) {
		if (temp)
			free(temp);
		if (c_Thread_arg)
			free(c_Thread_arg);
		fprintf (stderr, "cracking_MT(): malloc() failed.\n");
		exit(EXIT_FAILURE);
	}


	/* initialize crackThread arguments */
	if (alt == 1) {
		/* Alternative 1: each thread cracks one consecutive slice */
		for (i = 0, f = first, l = f + mm - 1; i < nthreads; i++, f += mm, l += mm) {
			c_Thread_arg[i].b       = b;
			c_Thread_arg[i].payload = payloadBuffer;
			c_Thread_arg[i].pivot    = &pivot;
			c_Thread_arg[i].pivotR = pivot_P;
			c_Thread_arg[i].first   = f;
			c_Thread_arg[i].last    = (i < nthreads - 1) ? l : last_vector_pos;
			c_Thread_arg[i].ml      = 0;
			c_Thread_arg[i].mr      = 0;
		}
	} else /* alt == 2 */ {
		/* Alternative 2: nthreads-1 threads crack two disjoint half-slices (from either end of the input piece), each - last thread cracks center consecutive slice */
		for (i = 0, f = first, l = last_vector_pos; i < nthreads; i++, f += ml, l -= mr) {
			c_Thread_arg[i].b       = b;
			c_Thread_arg[i].payload = payloadBuffer;
			c_Thread_arg[i].pivot    = &pivot;
			c_Thread_arg[i].pivotR = pivot_P;
			c_Thread_arg[i].first   = f;
			c_Thread_arg[i].last    = l;
			c_Thread_arg[i].ml = (i < nthreads - 1) ? ml : 0;
			c_Thread_arg[i].mr = (i < nthreads - 1) ? mr : 0;
		}
	}

#if PCMON == 1
	PCM * m = PCM::getInstance();

	SystemCounterState before_sstate = m->getSystemCounterState();
#endif


	gettimeofday(&tvb, NULL);

	#pragma omp parallel for
	for (int i = 0; i < nthreads; ++i) {
		cracking_MT_vectorized_crackThread(&c_Thread_arg[i]);
	}

	gettimeofday(&tvc, NULL);

#if PCMON==1
	SystemCounterState after_sstate = m->getSystemCounterState();

	double l2 = getL2CacheHitRatio(before_sstate, after_sstate);
	double l3 = getL3CacheHitRatio(before_sstate, after_sstate);
	uint64_t r = getBytesReadFromMC(before_sstate, after_sstate);
	uint64_t w = getBytesWrittenToMC(before_sstate, after_sstate);
	printf("l2 hit %lf. l3 hit %lf. r %lu. w %lu\n", l2, l3, r, w);

#endif
	/* "meta-crack": move "wrong" parts of slices to correct final location */
	if (alt == 2) {
		/* Alt. 2: treat each half-slice as individual slice */
		for (i = 0, j = 2 * nthreads - 2; i < nthreads - 1; i++, j--) {
			assert(i < j - 1);
			assert(c_Thread_arg[i].ml > 0 && c_Thread_arg[i].mr > 0);
			c_Thread_arg[j].ml    = c_Thread_arg[i].mr;
			c_Thread_arg[j].last  = c_Thread_arg[i].last;
			c_Thread_arg[j].first = c_Thread_arg[i].last  - c_Thread_arg[i].mr + 1;
			c_Thread_arg[i].last  = c_Thread_arg[i].first + c_Thread_arg[i].ml - 1;
			assert(first <= c_Thread_arg[i].first);
			assert(c_Thread_arg[i].first < c_Thread_arg[i].last);
			assert(c_Thread_arg[i].last /*+ 1*/ < c_Thread_arg[j].first);
			assert(c_Thread_arg[j].first < c_Thread_arg[j].last);
			assert(c_Thread_arg[j].last <= last_vector_pos);
			if (c_Thread_arg[i].pos_r < c_Thread_arg[j].first) {
				c_Thread_arg[j].pos_r = c_Thread_arg[j].first;
			} else {
				c_Thread_arg[j].pos_r = c_Thread_arg[i].pos_r;
				c_Thread_arg[i].pos_r = c_Thread_arg[i].last + 1;
			}
			c_Thread_arg[j].ml = c_Thread_arg[i].mr = 0;
			c_Thread_arg[j].mr = c_Thread_arg[i].mr = 0;
		}
		assert(i == j);
	}
	i = 0;
	j = alt * nthreads - alt;
	while (i < j) {
		/* skip over entirely smaller slices from beginning */
		while (i < j && c_Thread_arg[i].pos_r > c_Thread_arg[i].last)
			i++;
		{
			/* skip over entirely larger slices from end */
			while (i < j && (c_Thread_arg[j].pos_r <=  c_Thread_arg[j].first))
				j--;
			if (i < j) {
				/* size of "wrong" part of left slice with larger values */
				const size_t si = c_Thread_arg[i].last + 1 - c_Thread_arg[i].pos_r;
				/* size of "wrong" part of right slice with smaller values */
				const size_t sj = c_Thread_arg[j].pos_r - c_Thread_arg[j].first;
				const size_t sk = si > sj ? sj : si;
				const size_t st = sk * sizeof(targetType);
				const size_t pi = c_Thread_arg[i].pos_r;
				const size_t pj = c_Thread_arg[j].pos_r - sk;

				assert(si <= mm);
				assert(sj <= mm);
				memcpy(temp, &b[pi], st);
				memcpy(&b[pi], &b[pj], st);
				memcpy(&b[pj], temp, st);
				c_Thread_arg[i].pos_r += sk;
				c_Thread_arg[j].pos_r -= sk;

				assert((si <= sj) == (c_Thread_arg[i].pos_r == c_Thread_arg[i].last + 1));
				assert((sj <= si) == (c_Thread_arg[j].pos_r == c_Thread_arg[j].first));

				i += (si <= sj);
				j -= (sj <= si);
			}
		}
	}

	/* determine pivot (This is useful only for result validation)*/
	f = BUN_NONE;
	for (i = 0, j = alt * nthreads - alt; i <= j; j--) {
		l = c_Thread_arg[j].pos_r;
		if (l == last_vector_pos + 1 || (l <= last_vector_pos && b[l] >= pivot && (l == first || b[l - 1] < pivot)))
		{
			if (f == BUN_NONE) {
				f = l;
			}
			else {
				assert(f == l);
			}
		}
	}

	/*The rest of the values if the elements are not divided by the vector size*/
	if (remaining_elements > 0)
	{
		abort(); // just to be aware when this code does actually run
		size_t qualifying_elements = 0;
		size_t lowerCursor = last_vector_pos + 1, upperCursor = last_vector_pos + remaining_elements;
		size_t *tmp = (size_t*)malloc(remaining_elements*sizeof(targetType));
		size_t *tmp_payload = (size_t*)malloc(remaining_elements*sizeof(payloadType));

		while (lowerCursor < upperCursor)
		{
			while (lowerCursor < upperCursor && b[lowerCursor] < pivot)
			{
				lowerCursor++;
				qualifying_elements++;
			}

			while (lowerCursor < upperCursor && b[upperCursor] >= pivot)
				upperCursor--;

			if (lowerCursor < upperCursor) {
				targetType tmp = b[lowerCursor];
				b[lowerCursor] = b[upperCursor];
				b[upperCursor] = tmp;
				lowerCursor++;
				upperCursor--;
				qualifying_elements++;
			}
		}

		if (lowerCursor == upperCursor && b[lowerCursor] < pivot)
			qualifying_elements += 1;

		if (qualifying_elements > 0)
		{

			if (f == last_vector_pos + 1) /*if there are not qualifying tuples*/
			{
				f = first;
			}

			if(qualifying_elements <= (last_vector_pos - f + 1))
			{
				naive_memcpy(tmp, b + f, sizeof(targetType)*qualifying_elements);
				naive_memcpy(b + f, b + last_vector_pos + 1, sizeof(targetType)*qualifying_elements);
				naive_memcpy(b + last_vector_pos + 1, tmp, sizeof(targetType)*qualifying_elements);
			}
			else
			{
				size_t copied_elements = last_vector_pos - f + 1;
				naive_memcpy(tmp, b + f, sizeof(targetType)*copied_elements);
				naive_memcpy(b + f, b + last_vector_pos + 1, sizeof(targetType)*copied_elements);
				naive_memcpy(b + last_vector_pos + 1, tmp, sizeof(targetType)*copied_elements);
			}
			f += qualifying_elements;
		}
		else if (qualifying_elements == 0 && f == last_vector_pos + 1)
			f += remaining_elements;

		free(tmp);
	}

	assert(f != BUN_NONE);
	assert((f == last + 1 && b[f-1] < pivot) || b[f]   >= pivot);
	assert((f == first    && b[f]   >= pivot) || b[f-1] < pivot);
	*pos = (size_t) (f == 0 ? 0 : f - 1);

	gettimeofday(&tvd, NULL);

	long int diffbc = timediff(tvb, tvc);
	long int diffcd = timediff(tvc, tvd);

#if TIMING == 1
	fprintf(stderr, "b to c: %07ld\nc to d: %07ld\n", diffbc, diffcd);
#endif

	free(temp);
	free(c_Thread_arg);
}
