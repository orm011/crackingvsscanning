/*
 * stdpartition.cc
 *
 *  Created on: Feb 19, 2015
 *      Author: orm
 */

#include "../Framework/interface.h"
#include <parallel/algorithm>

	class IsLessThan {
	  public:

	    IsLessThan (targetType x) : val(x) {}

	    bool operator() (targetType x) const
	    { return (x < val); }

	  private:
	    targetType val;
	};


targetType* performCrack(targetType* buffer, targetType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {


	__gnu_parallel::partition(buffer, buffer+bufferSize, IsLessThan(pivot));

	return NULL;
}
