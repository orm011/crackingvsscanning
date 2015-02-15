#include "../Framework/interface.h"

#include "common.h"
#include "immintrin.h"
#include "smmintrin.h"
#include "emmintrin.h"

#define vectorBitSize 256
typedef __m256i vectorType;
#define vectorInstructionType mm
#define vectorWordSize (sizeof(vectorType)/sizeof(unsigned int))

typedef union {
	vectorType native;
	unsigned int scalar[vectorWordSize];
} avx2word;

#define debug_assert if(0) assert
#define debug_printf if(0) printf

static inline avx2word changeLaneOfFastestCursor(avx2word leftyCursors, avx2word validityFlags, avx2word* strides, int* done, int sideIndex){
	char* side = ((char*[2]){"lefty", "righty"})[sideIndex];
	int slowestLane = 0;
	int fastestLane = 0;
	int slowestInvalidLane = -1;
	int slowestInvalidLaneCursor = INT_MAX;
	for (int i = 0; i < sizeof(avx2word)/sizeof(unsigned int); i++){
		if(leftyCursors.scalar[i] < leftyCursors.scalar[slowestLane])
			slowestLane = i;
		if(leftyCursors.scalar[i] > leftyCursors.scalar[fastestLane])
			fastestLane = i;
		
	}

	debug_printf("slowest %slane: %d (%d), fastest %slane: %d (%d)\n", side, slowestLane, leftyCursors.scalar[slowestLane], side, fastestLane, leftyCursors.scalar[fastestLane]);
	for (int i = 0; i < sizeof(avx2word)/sizeof(unsigned int); i++){
		if(validityFlags.scalar[i] == 0
			 && leftyCursors.scalar[i] < slowestInvalidLaneCursor){
			slowestInvalidLane = i;
			slowestInvalidLaneCursor = leftyCursors.scalar[i];
		}
	}
	if(slowestInvalidLane >= 0 && ! (leftyCursors.scalar[slowestInvalidLane] < ELEMENTS_PER_VECTOR) )
		done[0] |= 1<<sideIndex;
	{
		leftyCursors.scalar[fastestLane] = leftyCursors.scalar[slowestLane] + strides[0].scalar[slowestLane];
		strides[0].scalar[slowestLane] = strides[0].scalar[fastestLane] = 2*strides[0].scalar[slowestLane];
		debug_printf("changed %slanes to %d (%d), %d (%d), %d (%d), %d (%d)\n", side, leftyCursors.scalar[0], leftyCursors.scalar[0]%vectorWordSize, leftyCursors.scalar[1], leftyCursors.scalar[1]%vectorWordSize, leftyCursors.scalar[2], leftyCursors.scalar[2]%vectorWordSize, leftyCursors.scalar[3], leftyCursors.scalar[3]%vectorWordSize);
		debug_printf("changed %sstrides to %d, %d, %d, %d\n", side, strides[0].scalar[0], strides[0].scalar[1], strides[0].scalar[2], strides[0].scalar[3]);
		if(leftyCursors.scalar[fastestLane] < ELEMENTS_PER_VECTOR)
			done[0] |= 1<<sideIndex;
		for (int i = 0; i < vectorWordSize; i++) {
			if(strides[0].scalar[i] >= ELEMENTS_PER_VECTOR)
				strides[0].scalar[i] = 0;
		}

		return leftyCursors;
	}
	
	debug_printf("unchanged lanes: %d, %d, %d, %d\n", leftyCursors.scalar[0], leftyCursors.scalar[1], leftyCursors.scalar[2], leftyCursors.scalar[3]);

	return leftyCursors;
	
}

static inline int _mm256_test_all_ones(__m256i v){
	return _mm_test_all_ones (_mm256_extractf128_si256(v, 0))
		& _mm_test_all_ones (_mm256_extractf128_si256(v, 1));
}

static inline cursorDeltas performSIMDCrackOnVectors(const targetType* input, targetType* leftOutput, targetType* rightOutput, const targetType pivot){
	const unsigned int AVX_WORDS_PER_VECTOR = VECTORSIZE/sizeof(avx2word);
	int 
		rightOutI = 0,
		leftOutI = 0;
	avx2word* lefties = (avx2word*)leftOutput;
	avx2word* righties = (avx2word*)(rightOutput - vectorWordSize+1);
	avx2word pivots = {.scalar = {[0 ... vectorWordSize-1] = pivot}},	ones = {.scalar = {[0 ... vectorWordSize-1] = 1<<31}},	validbitmask = {.scalar = {[0 ... vectorWordSize-1] = 1<<31}};
	avx2word leftyStrides = {.scalar = {[0 ... vectorWordSize-1] = vectorWordSize}};
	avx2word rightyStrides = {.scalar = {[0 ... vectorWordSize-1] = vectorWordSize}};

	avx2word leftyCursors, invalidLefties = validbitmask;
	avx2word rightyCursors, invalidRighties = validbitmask;
	/* avx2word clip = {.scalar = {ELEMENTS_PER_VECTOR-4, ELEMENTS_PER_VECTOR-3, ELEMENTS_PER_VECTOR-2, ELEMENTS_PER_VECTOR-1 }}; */
	avx2word over;

	for (int i = 0; i < vectorWordSize; i++) {
    leftyCursors.scalar[i] = i;
    rightyCursors.scalar[i] = i;
		over.scalar[i] = ELEMENTS_PER_VECTOR-vectorWordSize+i+1;
	}

	int done = 0;
	debug_printf("processing vector from %p to %p...\n", input, input+ELEMENTS_PER_VECTOR);
	/* for(int inI = 0; inI<AVX_WORDS_PER_VECTOR; inI++) */
	while(done != 3){
		const avx2word gatheredLefties = {.native = _mm256_mask_i32gather_epi32 (lefties[leftOutI].native, input, leftyCursors.native, invalidLefties.native, sizeof(targetType))};
		const avx2word gatheredRighties = {.native = _mm256_mask_i32gather_epi32 (righties[rightOutI].native, input, rightyCursors.native, invalidRighties.native, sizeof(targetType))};
		lefties[leftOutI].native = gatheredLefties.native;
		righties[rightOutI].native = gatheredRighties.native;
		avx2word leftyValidityFlags = {.native = _mm256_cmpgt_epi32(pivots.native, gatheredLefties.native)};
		avx2word rightyValidityFlags = {.native = _mm256_or_si256(_mm256_cmpgt_epi32(gatheredRighties.native, pivots.native), _mm256_cmpeq_epi32(gatheredRighties.native, pivots.native))};
		const unsigned int  leftiesFilled = _mm256_test_all_ones (leftyValidityFlags.native);
		const unsigned int  rightiesFilled = _mm256_test_all_ones (rightyValidityFlags.native);
		leftOutI += leftiesFilled;
		rightOutI -= rightiesFilled;

		const avx2word leftyAdvance = {.native = _mm256_andnot_si256(leftyValidityFlags.native, leftyStrides.native)};
		const avx2word rightyAdvance = {.native = _mm256_andnot_si256(rightyValidityFlags.native, rightyStrides.native)};
		
		leftyCursors.native = _mm256_add_epi32 (leftyCursors.native,
																				 leftyAdvance.native);
		leftyCursors.native = _mm256_add_epi32 (_mm256_mullo_epi32(_mm256_set1_epi32 (leftiesFilled), leftyStrides.native), leftyCursors.native);

		rightyCursors.native = _mm256_add_epi32 (rightyCursors.native,
																				 rightyAdvance.native);
		rightyCursors.native = _mm256_add_epi32 (_mm256_mullo_epi32(_mm256_set1_epi32 (rightiesFilled), rightyStrides.native), rightyCursors.native);

		if(!_mm256_test_all_ones(_mm256_cmpgt_epi32(over.native, leftyCursors.native)))
			leftyCursors = changeLaneOfFastestCursor(leftyCursors, leftyValidityFlags, &leftyStrides, &done, 0);
		if(!_mm256_test_all_ones(_mm256_cmpgt_epi32(over.native, rightyCursors.native)))
			rightyCursors = changeLaneOfFastestCursor(rightyCursors, rightyValidityFlags, &rightyStrides, &done, 1);
			

		invalidLefties.native = _mm256_xor_si256(_mm256_cmpeq_epi32(ones.native, ones.native), leftyValidityFlags.native);
		invalidLefties.native = _mm256_or_si256 (_mm256_set1_epi32 (leftiesFilled<<31), invalidLefties.native);
		leftyCursors.native = _mm256_min_epi32 (leftyCursors.native, over.native);

		invalidRighties.native = _mm256_xor_si256(_mm256_cmpeq_epi32(ones.native, ones.native), rightyValidityFlags.native);
		invalidRighties.native = _mm256_or_si256 (_mm256_set1_epi32 (rightiesFilled<<31), invalidRighties.native);
		rightyCursors.native = _mm256_min_epi32 (rightyCursors.native, over.native);
		
	}
	return (cursorDeltas){.left = leftOutI*vectorWordSize, .right = rightOutI*vectorWordSize};
}

targetType* performCrack(targetType* restrict const buffer, targetType* restrict const payloadBuffer, size_t valueCount, targetType pivot,const targetType pivot_P) {
//TODO: implement the payload reshuffling
	debug_assert(valueCount%(2*ELEMENTS_PER_VECTOR) == 0);
	const size_t vectorCount = valueCount/ELEMENTS_PER_VECTOR;
	size_t lowerReadCursor = 0, upperReadCursor = valueCount - 1;
	size_t lowerWriteCursor = 0, upperWriteCursor = valueCount-1;
	targetType localBuffer[ELEMENTS_PER_VECTOR*3];

	__builtin_memcpy(localBuffer, buffer, sizeof(targetType)*2*ELEMENTS_PER_VECTOR);
	__builtin_memcpy(localBuffer+2*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);
	lowerReadCursor += 2*ELEMENTS_PER_VECTOR;
	upperReadCursor -= ELEMENTS_PER_VECTOR;

	for (size_t vectorI = 0; vectorI < vectorCount; vectorI++)	{
		cursorDeltas deltas = performSIMDCrackOnVectors(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer + lowerWriteCursor, buffer + upperWriteCursor, pivot);
		lowerWriteCursor += deltas.left;
		upperWriteCursor += deltas.right;
		if(lowerReadCursor-lowerWriteCursor < upperWriteCursor-upperReadCursor){
			__builtin_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+lowerReadCursor, sizeof(targetType)*ELEMENTS_PER_VECTOR);
			lowerReadCursor+=ELEMENTS_PER_VECTOR;
		} else {
			__builtin_memcpy(localBuffer+(vectorI%3)*ELEMENTS_PER_VECTOR, buffer+upperReadCursor-ELEMENTS_PER_VECTOR, sizeof(targetType)*ELEMENTS_PER_VECTOR);
			upperReadCursor-=ELEMENTS_PER_VECTOR;
		}
	}
	return NULL;
}
