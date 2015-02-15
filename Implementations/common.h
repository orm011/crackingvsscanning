#ifndef _COMMON_H_
#define _COMMON_H_

typedef struct {
	int left, right;
} cursorDeltas;

const size_t ELEMENTS_PER_VECTOR = VECTORSIZE/sizeof(targetType);

static inline cursorDeltas performCrackOnVectors(const targetType* restrict input, targetType* restrict leftOutput, targetType* restrict rightOutput, const targetType pivot){
	int 
		rightOutI = 0,
		leftOutI = 0,
		inI;
	for (inI = 0; inI < ELEMENTS_PER_VECTOR; inI++){
		rightOutput[rightOutI] = input[inI];
		const unsigned int  isLessThan = (input[inI] < pivot);
		rightOutI -= (~isLessThan)&1;
		leftOutput[leftOutI] = input[inI];
		leftOutI += isLessThan;
	}
	return (cursorDeltas){.left = leftOutI, .right = rightOutI};
}

#endif /* _COMMON_H_ */
